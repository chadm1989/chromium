// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "content/renderer/pepper/host_globals.h"
#include "content/renderer/pepper/host_var_tracker.h"
#include "content/renderer/pepper/mock_resource.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/pepper_try_catch.h"
#include "content/renderer/pepper/v8object_var.h"
#include "content/test/ppapi_unittest.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppp_instance.h"
#include "third_party/WebKit/public/web/WebBindings.h"

using ppapi::V8ObjectVar;

namespace content {

namespace {

int g_v8objects_alive = 0;

class MyObject : public gin::Wrappable<MyObject> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static v8::Handle<v8::Value> Create(v8::Isolate* isolate) {
    return gin::CreateHandle(isolate, new MyObject()).ToV8();
  }

 private:
  MyObject() { ++g_v8objects_alive; }
  virtual ~MyObject() { --g_v8objects_alive; }

  DISALLOW_COPY_AND_ASSIGN(MyObject);
};

gin::WrapperInfo MyObject::kWrapperInfo = {gin::kEmbedderNativeGin};

class PepperTryCatchForTest : public PepperTryCatch {
 public:
  explicit PepperTryCatchForTest(PepperPluginInstanceImpl* instance)
      : PepperTryCatch(instance, V8VarConverter::kAllowObjectVars),
        handle_scope_(instance->GetIsolate()),
        context_scope_(v8::Context::New(instance->GetIsolate())) {}

  virtual void SetException(const char* message) OVERRIDE { NOTREACHED(); }
  virtual v8::Handle<v8::Context> GetContext() OVERRIDE {
    return instance_->GetIsolate()->GetCurrentContext();
  }

 private:
  v8::HandleScope handle_scope_;
  v8::Context::Scope context_scope_;

  DISALLOW_COPY_AND_ASSIGN(PepperTryCatchForTest);
};

}  // namespace

class HostVarTrackerTest : public PpapiUnittest {
 public:
  HostVarTrackerTest() {}

  HostVarTracker& tracker() { return *HostGlobals::Get()->host_var_tracker(); }
};

TEST_F(HostVarTrackerTest, DeleteObjectVarWithInstance) {
  v8::Isolate* test_isolate = v8::Isolate::GetCurrent();

  // Make a second instance (the test harness already creates & manages one).
  scoped_refptr<PepperPluginInstanceImpl> instance2(
      PepperPluginInstanceImpl::Create(NULL, module(), NULL, GURL()));
  PP_Instance pp_instance2 = instance2->pp_instance();

  {
    PepperTryCatchForTest try_catch(instance2.get());
    // Make an object var.
    ppapi::ScopedPPVar var = try_catch.FromV8(MyObject::Create(test_isolate));
    EXPECT_EQ(1, g_v8objects_alive);
    EXPECT_EQ(1, tracker().GetLiveV8ObjectVarsForTest(pp_instance2));
    // Purposely leak the var.
    var.Release();
  }

  // Free the instance, this should release the ObjectVar.
  instance2 = NULL;
  EXPECT_EQ(0, tracker().GetLiveV8ObjectVarsForTest(pp_instance2));
  test_isolate->RequestGarbageCollectionForTesting(
      v8::Isolate::kFullGarbageCollection);
  EXPECT_EQ(0, g_v8objects_alive);
}

// Make sure that using the same NPObject should give the same PP_Var
// each time.
TEST_F(HostVarTrackerTest, ReuseVar) {
  PepperTryCatchForTest try_catch(instance());

  v8::Handle<v8::Value> v8_object = MyObject::Create(v8::Isolate::GetCurrent());
  ppapi::ScopedPPVar pp_object1 = try_catch.FromV8(v8_object);
  ppapi::ScopedPPVar pp_object2 = try_catch.FromV8(v8_object);

  // The two results should be the same.
  EXPECT_EQ(pp_object1.get().value.as_id, pp_object2.get().value.as_id);

  // The objects should be able to get us back to the associated v8 object.
  {
    scoped_refptr<V8ObjectVar> check_object(
        V8ObjectVar::FromPPVar(pp_object1.get()));
    ASSERT_TRUE(check_object.get());
    EXPECT_EQ(instance(), check_object->instance());
    EXPECT_EQ(v8_object, check_object->GetHandle());
  }

  // Remove both of the refs we made above.
  pp_object1 = ppapi::ScopedPPVar();
  pp_object2 = ppapi::ScopedPPVar();

  // Releasing the resource should free the internal ref, and so making a new
  // one now should generate a new ID.
  ppapi::ScopedPPVar pp_object3 = try_catch.FromV8(v8_object);
  EXPECT_NE(pp_object1.get().value.as_id, pp_object3.get().value.as_id);
}

}  // namespace content
