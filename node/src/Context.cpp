// Copyright 2021 The WebNN-native Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Context.h"

#include <napi.h>
#include <webnn/webnn_proc.h>
#include <webnn_native/WebnnNative.h>
#include <iostream>

Napi::FunctionReference node::Context::constructor;

namespace node {

    Context::Context(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Context>(info) {
        MLContextOptions options = {MLDevicePreference_Default, MLPowerPreference_Default};
        if (info.Length() > 0) {
            Napi::Object optionsObject = info[0].As<Napi::Object>();
            if (optionsObject.Has("powerPreference")) {
                if (!optionsObject.Get("powerPreference").IsString()) {
                    Napi::Error::New(info.Env(), "Invaild powerPreference")
                        .ThrowAsJavaScriptException();
                    return;
                }
                std::string powerPreference = optionsObject.Get("powerPreference").ToString();
                if (powerPreference == "default") {
                    options.powerPreference = MLPowerPreference_Default;
                } else if (powerPreference == "low-power") {
                    options.powerPreference = MLPowerPreference_Low_power;
                } else if (powerPreference == "high-performance") {
                    options.powerPreference = MLPowerPreference_High_performance;
                } else {
                    Napi::Error::New(info.Env(), "Invaild powerPreference")
                        .ThrowAsJavaScriptException();
                    return;
                }
            }

            if (optionsObject.Has("devicePreference")) {
                if (!optionsObject.Get("devicePreference").IsString()) {
                    Napi::Error::New(info.Env(), "Invaild devicePreference")
                        .ThrowAsJavaScriptException();
                    return;
                }
                std::string devicePreference = optionsObject.Get("devicePreference").ToString();
                if (devicePreference == "default") {
                    options.devicePreference = MLDevicePreference_Default;
                } else if (devicePreference == "gpu") {
                    options.devicePreference = MLDevicePreference_Gpu;
                } else if (devicePreference == "cpu") {
                    options.devicePreference = MLDevicePreference_Cpu;
                } else {
                    Napi::Error::New(info.Env(), "Invaild devicePreference")
                        .ThrowAsJavaScriptException();
                    return;
                }
            }
        }

        WebnnProcTable backendProcs = webnn_native::GetProcs();
        webnnProcSetProcs(&backendProcs);
        mImpl = ml::Context::Acquire(webnn_native::CreateContext(&options));
        if (!mImpl) {
            Napi::Error::New(info.Env(), "Failed to create Context").ThrowAsJavaScriptException();
            return;
        }
        mImpl.SetUncapturedErrorCallback(
            [](MLErrorType type, char const* message, void* userData) {
                if (type != MLErrorType_NoError) {
                    std::cout << "Uncaptured Error type is " << type << ", message is " << message
                              << std::endl;
                }
            },
            this);
    }

    ml::Context Context::GetImpl() {
        return mImpl;
    }

    Napi::Object Context::Initialize(Napi::Env env, Napi::Object exports) {
        Napi::HandleScope scope(env);
        Napi::Function func = DefineClass(env, "MLContext", {});
        constructor = Napi::Persistent(func);
        constructor.SuppressDestruct();
        exports.Set("MLContext", func);
        return exports;
    }
}  // namespace node
