// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/resource_pool.h"

#include <stddef.h>

#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/resources/resource_util.h"
#include "cc/resources/scoped_resource.h"
#include "cc/test/fake_output_surface.h"
#include "cc/test/fake_output_surface_client.h"
#include "cc/test/fake_resource_provider.h"
#include "cc/test/test_shared_bitmap_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

class ResourcePoolTest : public testing::Test {
 public:
  void SetUp() override {
    output_surface_ = FakeOutputSurface::Create3d();
    ASSERT_TRUE(output_surface_->BindToClient(&output_surface_client_));
    shared_bitmap_manager_.reset(new TestSharedBitmapManager());
    resource_provider_ = FakeResourceProvider::Create(
        output_surface_.get(), shared_bitmap_manager_.get());
    task_runner_ = base::ThreadTaskRunnerHandle::Get();
    resource_pool_ =
        ResourcePool::Create(resource_provider_.get(), task_runner_.get());
  }

 protected:
  FakeOutputSurfaceClient output_surface_client_;
  std::unique_ptr<FakeOutputSurface> output_surface_;
  std::unique_ptr<SharedBitmapManager> shared_bitmap_manager_;
  std::unique_ptr<ResourceProvider> resource_provider_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<ResourcePool> resource_pool_;
};

TEST_F(ResourcePoolTest, AcquireRelease) {
  gfx::Size size(100, 100);
  ResourceFormat format = RGBA_8888;
  Resource* resource = resource_pool_->AcquireResource(size, format);
  EXPECT_EQ(size, resource->size());
  EXPECT_EQ(format, resource->format());
  EXPECT_TRUE(resource_provider_->CanLockForWrite(resource->id()));

  resource_pool_->ReleaseResource(resource);
}

TEST_F(ResourcePoolTest, AccountingSingleResource) {
  // Limits high enough to not be hit by this test.
  size_t bytes_limit = 10 * 1024 * 1024;
  size_t count_limit = 100;
  resource_pool_->SetResourceUsageLimits(bytes_limit, count_limit);

  gfx::Size size(100, 100);
  ResourceFormat format = RGBA_8888;
  size_t resource_bytes =
      ResourceUtil::UncheckedSizeInBytes<size_t>(size, format);
  Resource* resource = resource_pool_->AcquireResource(size, format);

  EXPECT_EQ(resource_bytes, resource_pool_->GetTotalMemoryUsageForTesting());
  EXPECT_EQ(resource_bytes, resource_pool_->memory_usage_bytes());
  EXPECT_EQ(1u, resource_pool_->GetTotalResourceCountForTesting());
  EXPECT_EQ(1u, resource_pool_->resource_count());
  EXPECT_EQ(0u, resource_pool_->GetBusyResourceCountForTesting());

  resource_pool_->ReleaseResource(resource);
  EXPECT_EQ(resource_bytes, resource_pool_->GetTotalMemoryUsageForTesting());
  EXPECT_EQ(1u, resource_pool_->GetTotalResourceCountForTesting());
  EXPECT_EQ(1u, resource_pool_->GetBusyResourceCountForTesting());

  resource_pool_->CheckBusyResources();
  EXPECT_EQ(resource_bytes, resource_pool_->GetTotalMemoryUsageForTesting());
  EXPECT_EQ(0u, resource_pool_->memory_usage_bytes());
  EXPECT_EQ(1u, resource_pool_->GetTotalResourceCountForTesting());
  EXPECT_EQ(0u, resource_pool_->resource_count());
  EXPECT_EQ(0u, resource_pool_->GetBusyResourceCountForTesting());

  resource_pool_->SetResourceUsageLimits(0u, 0u);
  resource_pool_->ReduceResourceUsage();
  EXPECT_EQ(0u, resource_pool_->GetTotalMemoryUsageForTesting());
  EXPECT_EQ(0u, resource_pool_->memory_usage_bytes());
  EXPECT_EQ(0u, resource_pool_->GetTotalResourceCountForTesting());
  EXPECT_EQ(0u, resource_pool_->resource_count());
  EXPECT_EQ(0u, resource_pool_->GetBusyResourceCountForTesting());
}

TEST_F(ResourcePoolTest, SimpleResourceReuse) {
  // Limits high enough to not be hit by this test.
  size_t bytes_limit = 10 * 1024 * 1024;
  size_t count_limit = 100;
  resource_pool_->SetResourceUsageLimits(bytes_limit, count_limit);

  gfx::Size size(100, 100);
  ResourceFormat format = RGBA_8888;

  Resource* resource = resource_pool_->AcquireResource(size, format);
  resource_pool_->ReleaseResource(resource);
  resource_pool_->CheckBusyResources();
  EXPECT_EQ(1u, resource_provider_->num_resources());

  // Same size/format should re-use resource.
  resource = resource_pool_->AcquireResource(size, format);
  EXPECT_EQ(1u, resource_provider_->num_resources());
  resource_pool_->ReleaseResource(resource);
  resource_pool_->CheckBusyResources();
  EXPECT_EQ(1u, resource_provider_->num_resources());

  // Different size/format should alloate new resource.
  resource = resource_pool_->AcquireResource(gfx::Size(50, 50), LUMINANCE_8);
  EXPECT_EQ(2u, resource_provider_->num_resources());
  resource_pool_->ReleaseResource(resource);
  resource_pool_->CheckBusyResources();
  EXPECT_EQ(2u, resource_provider_->num_resources());
}

TEST_F(ResourcePoolTest, LostResource) {
  // Limits high enough to not be hit by this test.
  size_t bytes_limit = 10 * 1024 * 1024;
  size_t count_limit = 100;
  resource_pool_->SetResourceUsageLimits(bytes_limit, count_limit);

  gfx::Size size(100, 100);
  ResourceFormat format = RGBA_8888;

  Resource* resource = resource_pool_->AcquireResource(size, format);
  EXPECT_EQ(1u, resource_provider_->num_resources());

  resource_provider_->LoseResourceForTesting(resource->id());
  resource_pool_->ReleaseResource(resource);
  resource_pool_->CheckBusyResources();
  EXPECT_EQ(0u, resource_provider_->num_resources());
}

TEST_F(ResourcePoolTest, BusyResourcesEventuallyFreed) {
  // Limits high enough to not be hit by this test.
  size_t bytes_limit = 10 * 1024 * 1024;
  size_t count_limit = 100;
  resource_pool_->SetResourceUsageLimits(bytes_limit, count_limit);

  // Set a quick resource expiration delay so that this test doesn't take long
  // to run.
  resource_pool_->SetResourceExpirationDelayForTesting(
      base::TimeDelta::FromMilliseconds(10));

  gfx::Size size(100, 100);
  ResourceFormat format = RGBA_8888;

  Resource* resource = resource_pool_->AcquireResource(size, format);
  EXPECT_EQ(1u, resource_provider_->num_resources());
  EXPECT_EQ(40000u, resource_pool_->GetTotalMemoryUsageForTesting());
  EXPECT_EQ(1u, resource_pool_->resource_count());

  resource_pool_->ReleaseResource(resource);
  EXPECT_EQ(1u, resource_provider_->num_resources());
  EXPECT_EQ(40000u, resource_pool_->GetTotalMemoryUsageForTesting());
  EXPECT_EQ(0u, resource_pool_->memory_usage_bytes());
  EXPECT_EQ(1u, resource_pool_->GetBusyResourceCountForTesting());

  // Wait for our resource pool to evict resources. We expect resources to be
  // released within 10 ms, give the thread up to 200.
  base::RunLoop run_loop;
  task_runner_->PostDelayedTask(FROM_HERE, run_loop.QuitClosure(),
                                base::TimeDelta::FromMillisecondsD(200));
  run_loop.Run();

  EXPECT_EQ(0u, resource_provider_->num_resources());
  EXPECT_EQ(0u, resource_pool_->GetTotalMemoryUsageForTesting());
  EXPECT_EQ(0u, resource_pool_->memory_usage_bytes());
}

TEST_F(ResourcePoolTest, UnusedResourcesEventuallyFreed) {
  // Limits high enough to not be hit by this test.
  size_t bytes_limit = 10 * 1024 * 1024;
  size_t count_limit = 100;
  resource_pool_->SetResourceUsageLimits(bytes_limit, count_limit);

  // Set a quick resource expiration delay so that this test doesn't take long
  // to run.
  resource_pool_->SetResourceExpirationDelayForTesting(
      base::TimeDelta::FromMilliseconds(100));

  gfx::Size size(100, 100);
  ResourceFormat format = RGBA_8888;

  Resource* resource = resource_pool_->AcquireResource(size, format);
  EXPECT_EQ(1u, resource_provider_->num_resources());
  EXPECT_EQ(40000u, resource_pool_->GetTotalMemoryUsageForTesting());
  EXPECT_EQ(1u, resource_pool_->GetTotalResourceCountForTesting());
  EXPECT_EQ(1u, resource_pool_->resource_count());

  resource_pool_->ReleaseResource(resource);
  EXPECT_EQ(1u, resource_provider_->num_resources());
  EXPECT_EQ(40000u, resource_pool_->GetTotalMemoryUsageForTesting());
  EXPECT_EQ(1u, resource_pool_->GetTotalResourceCountForTesting());
  EXPECT_EQ(1u, resource_pool_->GetBusyResourceCountForTesting());

  // Transfer the resource from the busy pool to the unused pool.
  resource_pool_->CheckBusyResources();
  EXPECT_EQ(1u, resource_provider_->num_resources());
  EXPECT_EQ(40000u, resource_pool_->GetTotalMemoryUsageForTesting());
  EXPECT_EQ(1u, resource_pool_->GetTotalResourceCountForTesting());
  EXPECT_EQ(0u, resource_pool_->resource_count());
  EXPECT_EQ(0u, resource_pool_->GetBusyResourceCountForTesting());

  // Wait for our resource pool to evict resources. We expect resources to be
  // released within 100 ms, give the thread up to 200.
  base::RunLoop run_loop;
  task_runner_->PostDelayedTask(FROM_HERE, run_loop.QuitClosure(),
                                base::TimeDelta::FromMillisecondsD(200));
  run_loop.Run();

  EXPECT_EQ(0u, resource_provider_->num_resources());
  EXPECT_EQ(0u, resource_pool_->GetTotalMemoryUsageForTesting());
}

TEST_F(ResourcePoolTest, UpdateContentId) {
  gfx::Size size(100, 100);
  ResourceFormat format = RGBA_8888;
  uint64_t content_id = 42;
  uint64_t new_content_id = 43;
  gfx::Rect new_invalidated_rect(20, 20, 10, 10);

  Resource* resource = resource_pool_->AcquireResource(size, format);
  resource_pool_->OnContentReplaced(resource->id(), content_id);
  resource_pool_->ReleaseResource(resource);
  resource_pool_->CheckBusyResources();

  // Ensure that we can retrieve the resource based on |content_id|.
  gfx::Rect invalidated_rect;
  Resource* reacquired_resource =
      resource_pool_->TryAcquireResourceForPartialRaster(
          new_content_id, new_invalidated_rect, content_id, &invalidated_rect);
  EXPECT_EQ(resource, reacquired_resource);
  EXPECT_EQ(new_invalidated_rect, invalidated_rect);
  resource_pool_->ReleaseResource(reacquired_resource);
}

TEST_F(ResourcePoolTest, UpdateContentIdAndInvalidatedRect) {
  gfx::Size size(100, 100);
  ResourceFormat format = RGBA_8888;
  uint64_t content_ids[] = {42, 43, 44};
  gfx::Rect invalidated_rect(20, 20, 10, 10);
  gfx::Rect second_invalidated_rect(25, 25, 10, 10);
  gfx::Rect expected_total_invalidated_rect(20, 20, 15, 15);

  // Acquire a new resource with the first content id.
  Resource* resource = resource_pool_->AcquireResource(size, format);
  resource_pool_->OnContentReplaced(resource->id(), content_ids[0]);

  // Attempt to acquire this resource. It is in use, so its ID and invalidated
  // rect should be updated, but a new resource will be returned.
  gfx::Rect new_invalidated_rect;
  Resource* reacquired_resource =
      resource_pool_->TryAcquireResourceForPartialRaster(
          content_ids[1], invalidated_rect, content_ids[0],
          &new_invalidated_rect);
  EXPECT_EQ(nullptr, reacquired_resource);
  EXPECT_EQ(gfx::Rect(), new_invalidated_rect);

  // Release the original resource, returning it to the unused pool.
  resource_pool_->ReleaseResource(resource);
  resource_pool_->CheckBusyResources();

  // Ensure that we cannot retrieve a resource based on the original content id.
  reacquired_resource = resource_pool_->TryAcquireResourceForPartialRaster(
      content_ids[1], invalidated_rect, content_ids[0], &new_invalidated_rect);
  EXPECT_EQ(nullptr, reacquired_resource);
  EXPECT_EQ(gfx::Rect(), new_invalidated_rect);

  // Ensure that we can retrieve the resource based on the second (updated)
  // content ID and that it has the expected invalidated rect.
  gfx::Rect total_invalidated_rect;
  reacquired_resource = resource_pool_->TryAcquireResourceForPartialRaster(
      content_ids[2], second_invalidated_rect, content_ids[1],
      &total_invalidated_rect);
  EXPECT_EQ(resource, reacquired_resource);
  EXPECT_EQ(expected_total_invalidated_rect, total_invalidated_rect);
  resource_pool_->ReleaseResource(reacquired_resource);
}

}  // namespace
}  // namespace cc
