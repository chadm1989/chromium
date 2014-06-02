// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_job_coordinator.h"

#include "base/memory/scoped_ptr.h"
#include "base/stl_util.h"
#include "content/browser/service_worker/service_worker_register_job_base.h"

namespace content {

ServiceWorkerJobCoordinator::JobQueue::JobQueue() {}

ServiceWorkerJobCoordinator::JobQueue::~JobQueue() {
  DCHECK(jobs_.empty()) << "Destroying JobQueue with " << jobs_.size()
                        << " unfinished jobs";
  STLDeleteElements(&jobs_);
}

ServiceWorkerRegisterJobBase* ServiceWorkerJobCoordinator::JobQueue::Push(
    scoped_ptr<ServiceWorkerRegisterJobBase> job) {
  if (jobs_.empty()) {
    job->Start();
    jobs_.push_back(job.release());
  } else if (!job->Equals(jobs_.back())) {
    jobs_.push_back(job.release());
  }
  // Note we are releasing 'job' here.

  DCHECK(!jobs_.empty());
  return jobs_.back();
}

void ServiceWorkerJobCoordinator::JobQueue::Pop(
    ServiceWorkerRegisterJobBase* job) {
  DCHECK(job == jobs_.front());
  jobs_.pop_front();
  delete job;
  if (!jobs_.empty())
    jobs_.front()->Start();
}

void ServiceWorkerJobCoordinator::JobQueue::ClearForShutdown() {
  STLDeleteElements(&jobs_);
}

ServiceWorkerJobCoordinator::ServiceWorkerJobCoordinator(
    base::WeakPtr<ServiceWorkerContextCore> context)
    : context_(context) {
}

ServiceWorkerJobCoordinator::~ServiceWorkerJobCoordinator() {
  if (!context_) {
    for (RegistrationJobMap::iterator it = jobs_.begin(); it != jobs_.end();
         ++it) {
      it->second.ClearForShutdown();
    }
    jobs_.clear();
  }
  DCHECK(jobs_.empty()) << "Destroying ServiceWorkerJobCoordinator with "
                        << jobs_.size() << " job queues";
}

void ServiceWorkerJobCoordinator::Register(
    const GURL& pattern,
    const GURL& script_url,
    int source_process_id,
    const ServiceWorkerRegisterJob::RegistrationCallback& callback) {
  scoped_ptr<ServiceWorkerRegisterJobBase> job(
      new ServiceWorkerRegisterJob(context_, pattern, script_url));
  ServiceWorkerRegisterJob* queued_job =
      static_cast<ServiceWorkerRegisterJob*>(jobs_[pattern].Push(job.Pass()));
  queued_job->AddCallback(callback, source_process_id);
}

void ServiceWorkerJobCoordinator::Unregister(
    const GURL& pattern,
    const ServiceWorkerUnregisterJob::UnregistrationCallback& callback) {
  scoped_ptr<ServiceWorkerRegisterJobBase> job(
      new ServiceWorkerUnregisterJob(context_, pattern));
  ServiceWorkerUnregisterJob* queued_job =
      static_cast<ServiceWorkerUnregisterJob*>(jobs_[pattern].Push(job.Pass()));
  queued_job->AddCallback(callback);
}

void ServiceWorkerJobCoordinator::FinishJob(const GURL& pattern,
                                            ServiceWorkerRegisterJobBase* job) {
  RegistrationJobMap::iterator pending_jobs = jobs_.find(pattern);
  DCHECK(pending_jobs != jobs_.end()) << "Deleting non-existent job.";
  pending_jobs->second.Pop(job);
  if (pending_jobs->second.empty())
    jobs_.erase(pending_jobs);
}

}  // namespace content
