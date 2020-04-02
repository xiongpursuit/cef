// Copyright (c) 2020 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_LIBCEF_BROWSER_MEDIA_ROUTER_MEDIA_SINK_IMPL_H_
#define CEF_LIBCEF_BROWSER_MEDIA_ROUTER_MEDIA_SINK_IMPL_H_
#pragma once

#include "include/cef_media_router.h"

#include "chrome/common/media_router/media_sink.h"

// Implementation of the CefMediaSink interface. May be created on any thread.
class CefMediaSinkImpl : public CefMediaSink {
 public:
  explicit CefMediaSinkImpl(const media_router::MediaSink& sink);
  CefMediaSinkImpl(const media_router::MediaSink::Id& sink_id,
                   const std::string& sink_name);

  // CefMediaSink methods.
  CefString GetId() override;
  bool IsValid() override;
  CefString GetName() override;
  CefString GetDescription() override;
  bool IsCastSink() override;
  bool IsDialSink() override;
  bool IsCompatibleWith(CefRefPtr<CefMediaSource> source) override;

  const media_router::MediaSink& sink() const { return sink_; }

 private:
  // Read-only after creation.
  const media_router::MediaSink sink_;

  IMPLEMENT_REFCOUNTING(CefMediaSinkImpl);
  DISALLOW_COPY_AND_ASSIGN(CefMediaSinkImpl);
};

#endif  // CEF_LIBCEF_BROWSER_MEDIA_ROUTER_MEDIA_SINK_IMPL_H_