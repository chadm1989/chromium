/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "wtf/PartitionAlloc.h"

#include "wtf/PageAllocator.h"
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

// A super page is at least 4 partition pages in order to make re-entrancy considerations simpler in partitionAllocPage().
COMPILE_ASSERT(WTF::kPartitionPageSize * 4 <= WTF::kSuperPageSize, ok_partition_page_size);
COMPILE_ASSERT(!(WTF::kSuperPageSize % WTF::kPartitionPageSize), ok_partition_page_multiple);
COMPILE_ASSERT(!(WTF::kPartitionPageSize % WTF::kSubPartitionPageSize), ok_sub_partition_page_multiple);

namespace WTF {

WTF_EXPORT void partitionAllocInit(PartitionRoot* root, size_t numBuckets, size_t maxAllocation)
{
    ASSERT(!root->initialized);
    root->initialized = true;
    root->lock = 0;
    root->totalSizeOfSuperPages = 0;
    root->numBuckets = numBuckets;
    root->maxAllocation = maxAllocation;
    size_t i;
    for (i = 0; i < root->numBuckets; ++i) {
        PartitionBucket* bucket = &root->buckets()[i];
        bucket->root = root;
        bucket->currPage = &root->seedPage;
        bucket->freePages = 0;
        bucket->numFullPages = 0;
    }

    root->nextSuperPage = 0;
    root->nextPartitionPage = 0;
    root->nextPartitionPageEnd = 0;
    root->currentExtent = &root->firstExtent;
    root->firstExtent.superPageBase = 0;
    root->firstExtent.superPagesEnd = 0;
    root->firstExtent.next = 0;
    root->seedPage.numAllocatedSlots = 0;
    root->seedPage.bucket = &root->seedBucket;
    root->seedPage.freelistHead = 0;
    root->seedPage.next = &root->seedPage;
    root->seedPage.prev = &root->seedPage;

    root->seedBucket.root = root;
    root->seedBucket.currPage = 0;
    root->seedBucket.freePages = 0;
    root->seedBucket.numFullPages = 0;
}

static bool partitionAllocShutdownBucket(PartitionBucket* bucket)
{
    // Failure here indicates a memory leak.
    bool noLeaks = !bucket->numFullPages;
    PartitionFreepagelistEntry* entry = bucket->freePages;
    while (entry) {
        PartitionFreepagelistEntry* next = entry->next;
        partitionFree(entry);
        entry = next;
    }
    PartitionPageHeader* page = bucket->currPage;
    do {
        if (page->numAllocatedSlots)
            noLeaks = false;
        page = page->next;
    } while (page != bucket->currPage);

    return noLeaks;
}

bool partitionAllocShutdown(PartitionRoot* root)
{
    bool noLeaks = true;
    ASSERT(root->initialized);
    root->initialized = false;
    size_t i;
    // First, free the non-metadata buckets. Freeing the free pages in these
    // buckets will depend on the metadata bucket.
    for (i = 0; i < root->numBuckets; ++i) {
        if (i != kInternalMetadataBucket) {
            PartitionBucket* bucket = &root->buckets()[i];
            if (!partitionAllocShutdownBucket(bucket))
                noLeaks = false;
        }
    }
    // Finally, free the freepage bucket.
    (void) partitionAllocShutdownBucket(&root->buckets()[kInternalMetadataBucket]);

    // Now that we've examined all partition pages in all buckets, it's safe
    // to free all our super pages. We first collect the super page pointers
    // on the stack because some of them are themselves store in super pages.
    char* superPages[kMaxPartitionSize / kSuperPageSize];
    size_t numSuperPages = 0;
    PartitionSuperPageExtentEntry* entry = &root->firstExtent;
    while (entry) {
        char* superPage = entry->superPageBase;
        while (superPage != entry->superPagesEnd) {
            superPages[numSuperPages] = superPage;
            numSuperPages++;
            superPage += kSuperPageSize;
        }
        entry = entry->next;
    }
    ASSERT(numSuperPages == root->totalSizeOfSuperPages / kSuperPageSize);
    for (size_t i = 0; i < numSuperPages; ++i)
        freeSuperPages(superPages[i], kSuperPageSize);

    return noLeaks;
}

static ALWAYS_INLINE PartitionPageHeader* partitionAllocPage(PartitionRoot* root)
{
    if (LIKELY(root->nextPartitionPage != 0)) {
        // In this case, we can still hand out pages from a previous
        // super page allocation.
        char* ret = root->nextPartitionPage;
        root->nextPartitionPage += kPartitionPageSize;
        if (UNLIKELY(root->nextPartitionPage == root->nextPartitionPageEnd)) {
            // We ran out, need to get more pages next time.
            root->nextPartitionPage = 0;
            root->nextPartitionPageEnd = 0;
        }
        return reinterpret_cast<PartitionPageHeader*>(ret);
    }

    // Need a new super page.
    root->totalSizeOfSuperPages += kSuperPageSize;
    RELEASE_ASSERT(root->totalSizeOfSuperPages <= kMaxPartitionSize);
    // We need to put a guard page in front if either:
    // a) This is the first super page allocation.
    // b) The super page did not end up at our suggested address.
    bool needsGuard = false;
    if (UNLIKELY(root->nextSuperPage == 0)) {
        needsGuard = true;
        root->nextSuperPage = getRandomSuperPageBase();
    }
    char* superPage = reinterpret_cast<char*>(allocSuperPages(root->nextSuperPage, kSuperPageSize));
    char* ret = superPage;
    if (superPage != root->nextSuperPage) {
        needsGuard = true;
        // Re-randomize the base location for next time just in case the
        // underlying operating system picks lousy locations for mappings.
        root->nextSuperPage = 0;
    } else {
        root->nextSuperPage = superPage + kSuperPageSize;
    }
    root->nextPartitionPageEnd = superPage + kSuperPageSize;
    if (needsGuard) {
        setSystemPagesInaccessible(superPage, kPartitionPageSize);
        ret += kPartitionPageSize;
    }
    root->nextPartitionPage = ret + kPartitionPageSize;

    // We allocated a new super page so update super page metadata.
    // First check if this is a new extent or not.
    PartitionSuperPageExtentEntry* currentExtent = root->currentExtent;
    if (UNLIKELY(needsGuard)) {
        if (currentExtent->superPageBase) {
            // We already have a super page, so need to allocate metadata in the linked list.
            // It should be fine to re-enter the allocator here because:
            // - A fresh partition page is still available, even if we already consumed a guard page and one partition page from the new super page.
            // - Partition page metadata is consistent at this time.
            // - We ASSERT that no surprising state change occurs.
            PartitionSuperPageExtentEntry* newEntry = reinterpret_cast<PartitionSuperPageExtentEntry*>(partitionBucketAlloc(&root->buckets()[kInternalMetadataBucket]));
            ASSERT(root->currentExtent == currentExtent);
            newEntry->next = 0;
            currentExtent->next = newEntry;
            currentExtent = newEntry;
            root->currentExtent = newEntry;
        }
        currentExtent->superPageBase = superPage;
        currentExtent->superPagesEnd = superPage + kSuperPageSize;
    } else {
        // We allocated next to an existing extent so just nudge the size up a little.
        currentExtent->superPagesEnd += kSuperPageSize;
        ASSERT(ret >= currentExtent->superPageBase && ret < currentExtent->superPagesEnd);
    }

    return reinterpret_cast<PartitionPageHeader*>(ret);
}

static ALWAYS_INLINE void partitionUnusePage(PartitionPageHeader* page)
{
    decommitSystemPages(page, kPartitionPageSize);
}

static ALWAYS_INLINE size_t partitionBucketSlots(const PartitionBucket* bucket)
{
    COMPILE_ASSERT(!(sizeof(PartitionPageHeader) % sizeof(void*)), PartitionPageHeader_size_should_be_multiple_of_pointer_size);
    return (kPartitionPageSize - sizeof(PartitionPageHeader)) / partitionBucketSize(bucket);
}

static ALWAYS_INLINE void partitionPageReset(PartitionPageHeader* page, PartitionBucket* bucket)
{
    page->numAllocatedSlots = 0;
    page->numUnprovisionedSlots = partitionBucketSlots(bucket);
    ASSERT(page->numUnprovisionedSlots > 1);
    page->bucket = bucket;
    // NULLing the freelist is not strictly necessary but it makes an ASSERT in partitionPageFillFreelist simpler.
    page->freelistHead = 0;
}

static ALWAYS_INLINE char* partitionPageAllocAndFillFreelist(PartitionPageHeader* page)
{
    size_t numSlots = page->numUnprovisionedSlots;
    ASSERT(numSlots);
    PartitionBucket* bucket = page->bucket;
    // We should only get here when _every_ slot is either used or unprovisioned.
    // (The third state is "on the freelist". If we have a non-empty freelist, we should not get here.)
    ASSERT(numSlots + page->numAllocatedSlots == partitionBucketSlots(bucket));
    // Similarly, make explicitly sure that the freelist is empty.
    ASSERT(!page->freelistHead);

    size_t size = partitionBucketSize(bucket);
    char* base = reinterpret_cast<char*>(page);
    char* returnObject = base + sizeof(PartitionPageHeader) + (size * page->numAllocatedSlots);
    char* nextFreeObject = returnObject + size;
    char* subPageLimit = reinterpret_cast<char*>((reinterpret_cast<uintptr_t>(returnObject) + kSubPartitionPageMask) & ~kSubPartitionPageMask);

    size_t numNewFreelistEntries = 0;
    if (LIKELY(subPageLimit > nextFreeObject))
        numNewFreelistEntries = (subPageLimit - nextFreeObject) / size;

    // We always return an object slot -- that's the +1 below.
    // We do not neccessarily create any new freelist entries, because we cross sub page boundaries frequently for large bucket sizes.
    numSlots -= (numNewFreelistEntries + 1);
    page->numUnprovisionedSlots = numSlots;
    page->numAllocatedSlots++;

    if (LIKELY(numNewFreelistEntries)) {
        PartitionFreelistEntry* entry = reinterpret_cast<PartitionFreelistEntry*>(nextFreeObject);
        page->freelistHead = entry;
        while (--numNewFreelistEntries) {
            nextFreeObject += size;
            PartitionFreelistEntry* nextEntry = reinterpret_cast<PartitionFreelistEntry*>(nextFreeObject);
            entry->next = partitionFreelistMask(nextEntry);
            entry = nextEntry;
        }
        entry->next = partitionFreelistMask(0);
    } else {
        page->freelistHead = 0;
    }
    return returnObject;
}

static ALWAYS_INLINE void partitionUnlinkPage(PartitionPageHeader* page)
{
    ASSERT(page->prev->next == page);
    ASSERT(page->next->prev == page);

    page->next->prev = page->prev;
    page->prev->next = page->next;
}

static ALWAYS_INLINE void partitionLinkPage(PartitionPageHeader* newPage, PartitionPageHeader* prevPage)
{
    ASSERT(prevPage->prev->next == prevPage);
    ASSERT(prevPage->next->prev == prevPage);

    newPage->prev = prevPage;
    newPage->next = prevPage->next;

    prevPage->next->prev = newPage;
    prevPage->next = newPage;
}

void* partitionAllocSlowPath(PartitionBucket* bucket)
{
    // The slow path is called when the freelist is empty.
    PartitionPageHeader* page = bucket->currPage;
    PartitionPageHeader* next = page->next;
    ASSERT(page == &bucket->root->seedPage || (page->bucket == bucket && next->bucket == bucket));

    // First, see if the partition page still has capacity and if so, fill out
    // the freelist a little more.
    if (LIKELY(page->numUnprovisionedSlots))
        return partitionPageAllocAndFillFreelist(page);

    // Second, look for a page in our linked ring list of non-full pages.
    while (LIKELY(next != page)) {
        ASSERT(next->bucket == bucket);
        ASSERT(next->next->prev == next);
        ASSERT(next->prev->next == next);
        ASSERT(next != &bucket->root->seedPage);
        if (LIKELY(next->freelistHead != 0)) {
            bucket->currPage = next;
            PartitionFreelistEntry* ret = next->freelistHead;
            next->freelistHead = partitionFreelistMask(ret->next);
            next->numAllocatedSlots++;
            return ret;
        }
        // Pull this page out of the non-full page list, since it has no free
        // slots.
        // This tags the page as full so that free'ing can tell, and move
        // the page back into the non-full page list when appropriate.
        ASSERT(next->numAllocatedSlots == partitionBucketSlots(bucket));
        next->numAllocatedSlots = -next->numAllocatedSlots;
        partitionUnlinkPage(next);
        ++bucket->numFullPages;

        next = next->next;
    }

    // Second, look in our list of freed but reserved pages.
    PartitionPageHeader* newPage;
    PartitionFreepagelistEntry* pagelist = bucket->freePages;
    if (LIKELY(pagelist != 0)) {
        newPage = pagelist->page;
        bucket->freePages = pagelist->next;
        partitionFree(pagelist);
        ASSERT(page != &bucket->root->seedPage);
        partitionLinkPage(newPage, page);
    } else {
        // Third. If we get here, we need a brand new page.
        newPage = partitionAllocPage(bucket->root);
        if (UNLIKELY(page == &bucket->root->seedPage)) {
            // If this is the first page allocation to this bucket, then
            // fully replace the seed page. This avoids pointlessly iterating
            // over it.
            newPage->prev = newPage;
            newPage->next = newPage;
        } else {
            partitionLinkPage(newPage, page);
        }
    }

    bucket->currPage = newPage;
    partitionPageReset(newPage, bucket);
    return partitionPageAllocAndFillFreelist(newPage);
}

void partitionFreeSlowPath(PartitionPageHeader* page)
{
    PartitionBucket* bucket = page->bucket;
    if (LIKELY(page->numAllocatedSlots == 0)) {
        // Page became fully unused.
        // If it's the current page, leave it be so that we don't bounce a page
        // onto the free page list and immediately back out again.
        if (LIKELY(page == bucket->currPage))
            return;

        partitionUnlinkPage(page);
        partitionUnusePage(page);
        PartitionFreepagelistEntry* entry = static_cast<PartitionFreepagelistEntry*>(partitionBucketAlloc(&bucket->root->buckets()[kInternalMetadataBucket]));
        entry->page = page;
        entry->next = bucket->freePages;
        bucket->freePages = entry;
    } else {
        // Fully used page became partially used. It must be put back on the
        // non-full page list.
        partitionLinkPage(page, bucket->currPage);
        page->numAllocatedSlots = -page->numAllocatedSlots - 2;
        ASSERT(page->numAllocatedSlots == partitionBucketSlots(bucket) - 1);
        --bucket->numFullPages;
    }
}

void* partitionReallocGeneric(PartitionRoot* root, void* ptr, size_t newSize)
{
#if defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
    return realloc(ptr, newSize);
#else
    bool oldPtrIsInPartition = partitionPointerIsValid(root, ptr);
    size_t oldIndex;
    if (LIKELY(partitionPointerIsValid(root, ptr))) {
        PartitionBucket* bucket = partitionPointerToPage(ptr)->bucket;
        ASSERT(bucket->root == root);
        oldIndex = bucket - root->buckets();
    } else {
        oldIndex = root->numBuckets;
    }

    size_t newIndex = QuantizedAllocation::quantizedSize(newSize) >> kBucketShift;
    if (newIndex > root->numBuckets)
        newIndex = root->numBuckets;

    if (oldIndex == newIndex) {
        // Same bucket. But kNumBuckets indicates the fastMalloc "bucket" so in
        // that case we're not done; we have to forward to fastRealloc.
        if (oldIndex == root->numBuckets)
            return WTF::fastRealloc(ptr, newSize);
        return ptr;
    }
    // This realloc cannot be resized in-place. Sadness.
    void* ret = partitionAllocGeneric(root, newSize);
    size_t oldSize = oldIndex << kBucketShift;
    size_t copySize = oldSize;
    if (newSize < oldSize)
        copySize = newSize;
    memcpy(ret, ptr, copySize);
    partitionFreeGeneric(root, ptr);
    return ret;
#endif
}

#ifndef NDEBUG

void partitionDumpStats(const PartitionRoot& root)
{
    size_t i;
    size_t totalLive = 0;
    size_t totalResident = 0;
    size_t totalFreeable = 0;
    for (i = 0; i < root.numBuckets; ++i) {
        const PartitionBucket& bucket = root.buckets()[i];
        if (bucket.currPage == &bucket.root->seedPage && !bucket.freePages) {
            // Empty bucket with no freelist pages. Skip reporting it.
            continue;
        }
        size_t numFreePages = 0;
        const PartitionFreepagelistEntry* freePages = bucket.freePages;
        while (freePages) {
            ++numFreePages;
            freePages = freePages->next;
        }
        size_t bucketSlotSize = partitionBucketSize(&bucket);
        size_t bucketNumSlots = partitionBucketSlots(&bucket);
        size_t numActiveBytes = bucket.numFullPages * bucketSlotSize * bucketNumSlots;
        size_t numResidentBytes = 0;
        size_t numActivePages = 0;
        const PartitionPageHeader* page = bucket.currPage;
        do {
            if (page != &bucket.root->seedPage) {
                ++numActivePages;
                numActiveBytes += (page->numAllocatedSlots * bucketSlotSize);
                size_t pageBytesResident = ((bucketNumSlots - page->numUnprovisionedSlots) * bucketSlotSize) + sizeof(PartitionPageHeader);
                // Round up to sub page size.
                pageBytesResident = (pageBytesResident + kSubPartitionPageMask) & ~kSubPartitionPageMask;
                numResidentBytes += pageBytesResident;
            }
            page = page->next;
        } while (page != bucket.currPage);
        totalLive += numActiveBytes;
        totalResident += numResidentBytes;
        if (!numActiveBytes)
            totalFreeable += numResidentBytes;
        printf("bucket size %ld: %ld/%ld bytes, %ld/%ld/%ld full/active/free pages\n", bucketSlotSize, numActiveBytes, numResidentBytes, bucket.numFullPages, numActivePages, numFreePages);
    }
    printf("total live: %ld bytes\n", totalLive);
    printf("total resident: %ld bytes\n", totalResident);
    printf("total freeable: %ld bytes\n", totalFreeable);
    fflush(stdout);
}

#endif // !NDEBUG

} // namespace WTF

