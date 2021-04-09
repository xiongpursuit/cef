// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CEF_LIBCEF_BROWSER_BROWSER_CONTEXT_IMPL_H_
#define CEF_LIBCEF_BROWSER_BROWSER_CONTEXT_IMPL_H_
#pragma once

#include <set>
#include <vector>

#include "include/cef_request_context_handler.h"
#include "libcef/browser/iothread_state.h"
#include "libcef/browser/request_context_handler_map.h"

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "chrome/common/plugin.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "url/origin.h"

/*
// Classes used in request processing (network, storage, service, etc.):
//
// WC = WebContents
//  Content API representation of a browser. Created by BHI or the system (for
//  popups) and owned by BHI. Keeps a pointer to the content::BrowserContext.
//
// BHI = AlloyBrowserHostImpl
//  Implements the CefBrowser and CefBrowserHost interfaces which are exposed
//  to clients. References an RCI instance. Owns a WC. Lifespan is controlled
//  by client references and CefBrowserInfoManager (until the browser has
//  closed).
//
// RCI = CefRequestContextImpl
//  Implements the CefRequestContext interface which is exposed to clients.
//  Creates or references a BC. Lifespan is controlled by client references and
//  BrowserMainParts (for the global RCI).
//
// BC = CefBrowserContext
//   Is/owns the content::BrowserContext which is the entry point from WC.
//   Owns the IOTS and creates the SPI indirectly. Potentially shared by
//   multiple RCI. Deletes itself when no longer needed by RCI.
//
// SPI = content::StoragePartitionImpl
//   Owns storage-related objects like Quota, IndexedDB, Cache, etc. Created by
//   StoragePartitionImplMap::Get(). Life span is controlled indirectly by BC.
//
// IOTS = CefIOThreadState
//   Stores state for access on the IO thread. Life span is controlled by BC.
//
//
// Relationship diagram:
//   ref = reference (CefRefPtr/scoped_refptr)
//   own = ownership (std::unique_ptr)
//   ptr = raw pointer
//
// BHI -ref-> RCI -ptr-> BC -own-> SPI, IOTS
//                      ^
// BHI -own-> WC -ptr--/
//
//
// How shutdown works:
// 1. AlloyBrowserHostImpl::DestroyBrowser is called on the UI thread after the
//    browser is closed and deletes the WebContents.
// 1. AlloyBrowserHostImpl is destroyed on any thread when the last reference
//    is released.
// 2. CefRequestContextImpl is destroyed (possibly asynchronously) on the UI
//    thread when the last reference is released.
// 3. CefBrowserContext is destroyed on the UI thread when no longer needed
//    by any CefRequestContextImpl (via RemoveCefRequestContext).
// 4. CefIOThreadState is destroyed asynchronously on the IO thread after
//    the owning CefBrowserContext is destroyed.
*/

namespace content {
class BrowserContext;
}

class CefMediaRouterManager;
class CefRequestContextImpl;
class Profile;

// Main entry point for configuring behavior on a per-RequestContext basis. The
// content::BrowserContext represented by this class is passed to
// WebContents::Create in AlloyBrowserHostImpl::CreateInternal. Only accessed on
// the UI thread unless otherwise indicated.
class CefBrowserContext {
 public:
  // Returns the existing instance, if any, associated with the specified
  // |cache_path|.
  static CefBrowserContext* FromCachePath(const base::FilePath& cache_path);

  // Returns the existing instance, if any, associated with the specified IDs.
  // See comments on IsAssociatedContext() for usage.
  static CefBrowserContext* FromIDs(int render_process_id,
                                    int render_frame_id,
                                    int frame_tree_node_id,
                                    bool require_frame_match);

  // Returns the underlying CefBrowserContext if any.
  static CefBrowserContext* FromBrowserContext(
      const content::BrowserContext* context);
  static CefBrowserContext* FromProfile(const Profile* profile);

  // Returns all existing CefBrowserContext.
  static std::vector<CefBrowserContext*> GetAll();

  // Returns the content and chrome layer representations of the context.
  virtual content::BrowserContext* AsBrowserContext() = 0;
  virtual Profile* AsProfile() = 0;

  // Called from CefRequestContextImpl to track associated objects. This
  // object will delete itself when the count reaches zero.
  void AddCefRequestContext(CefRequestContextImpl* context);
  virtual void RemoveCefRequestContext(CefRequestContextImpl* context);

  // Called from CefRequestContextImpl::OnRenderFrameCreated.
  void OnRenderFrameCreated(CefRequestContextImpl* request_context,
                            int render_process_id,
                            int render_frame_id,
                            int frame_tree_node_id,
                            bool is_main_frame,
                            bool is_guest_view);

  // Called from CefRequestContextImpl::OnRenderFrameDeleted.
  void OnRenderFrameDeleted(CefRequestContextImpl* request_context,
                            int render_process_id,
                            int render_frame_id,
                            int frame_tree_node_id,
                            bool is_main_frame,
                            bool is_guest_view);

  // Returns the handler that matches the specified IDs. Pass -1 for unknown
  // values. If |require_frame_match| is true only exact matches will be
  // returned. If |require_frame_match| is false, and there is not an exact
  // match, then the first handler for the same |render_process_id| will be
  // returned.
  CefRefPtr<CefRequestContextHandler> GetHandler(
      int render_process_id,
      int render_frame_id,
      int frame_tree_node_id,
      bool require_frame_match) const;

  // Returns true if this context is associated with the specified IDs. Pass -1
  // for unknown values. If |require_frame_match| is true only exact matches
  // will qualify. If |require_frame_match| is false, and there is not an exact
  // match, then any match for |render_process_id| will qualify.
  bool IsAssociatedContext(int render_process_id,
                           int render_frame_id,
                           int frame_tree_node_id,
                           bool require_frame_match) const;

  // Remember the plugin load decision for plugin status requests that arrive
  // via CefPluginServiceFilter::IsPluginAvailable.
  void AddPluginLoadDecision(int render_process_id,
                             const base::FilePath& plugin_path,
                             bool is_main_frame,
                             const url::Origin& main_frame_origin,
                             chrome::mojom::PluginStatus status);
  bool HasPluginLoadDecision(int render_process_id,
                             const base::FilePath& plugin_path,
                             bool is_main_frame,
                             const url::Origin& main_frame_origin,
                             chrome::mojom::PluginStatus* status) const;

  // Clear the plugin load decisions associated with |render_process_id|, or all
  // plugin load decisions if |render_process_id| is -1.
  void ClearPluginLoadDecision(int render_process_id);

  // Called from CefRequestContextImpl methods of the same name.
  void RegisterSchemeHandlerFactory(const CefString& scheme_name,
                                    const CefString& domain_name,
                                    CefRefPtr<CefSchemeHandlerFactory> factory);
  void ClearSchemeHandlerFactories();
  // TODO(chrome-runtime): Make these extension methods pure virtual.
  virtual void LoadExtension(const CefString& root_directory,
                             CefRefPtr<CefDictionaryValue> manifest,
                             CefRefPtr<CefExtensionHandler> handler,
                             CefRefPtr<CefRequestContext> loader_context);
  virtual bool GetExtensions(std::vector<CefString>& extension_ids);
  virtual CefRefPtr<CefExtension> GetExtension(const CefString& extension_id);

  // Called from CefExtensionImpl::Unload().
  virtual bool UnloadExtension(const CefString& extension_id);

  // Returns true if this context supports print preview.
  virtual bool IsPrintPreviewSupported() const;

  network::mojom::NetworkContext* GetNetworkContext();

  CefMediaRouterManager* GetMediaRouterManager();

  using CookieableSchemes = base::Optional<std::vector<std::string>>;

  // Returns the schemes associated with this context specifically, or the
  // global configuration if unset.
  CookieableSchemes GetCookieableSchemes() const;
  static CookieableSchemes GetGlobalCookieableSchemes();

  // These accessors are safe to call from any thread because the values don't
  // change during this object's lifespan.
  const CefRequestContextSettings& settings() const { return settings_; }
  base::FilePath cache_path() const { return cache_path_; }
  scoped_refptr<CefIOThreadState> iothread_state() const {
    return iothread_state_;
  }

  // Used to hold a WeakPtr reference to this this object. The Getter returns
  // nullptr if this object has already been destroyed.
  using Getter = base::RepeatingCallback<CefBrowserContext*()>;
  Getter getter() const { return getter_; }

 protected:
  explicit CefBrowserContext(const CefRequestContextSettings& settings);
  virtual ~CefBrowserContext();

  // Will be called immediately after this object is created.
  virtual void Initialize();

  // Will be called immediately before this object is deleted.
  virtual void Shutdown();

  // Members initialized during construction or Initialize() are safe to access
  // from any thread.
  const CefRequestContextSettings settings_;
  base::FilePath cache_path_;

 private:
  scoped_refptr<CefIOThreadState> iothread_state_;
  CookieableSchemes cookieable_schemes_;
  std::unique_ptr<CefMediaRouterManager> media_router_manager_;

  // CefRequestContextImpl objects referencing this object.
  std::set<CefRequestContextImpl*> request_context_set_;

  // Map IDs to CefRequestContextHandler objects.
  CefRequestContextHandlerMap handler_map_;

  // Map (render_process_id, plugin_path, is_main_frame, main_frame_origin) to
  // plugin load decision.
  typedef std::map<
      std::pair<std::pair<int, base::FilePath>, std::pair<bool, url::Origin>>,
      chrome::mojom::PluginStatus>
      PluginLoadDecisionMap;
  PluginLoadDecisionMap plugin_load_decision_map_;

  // Set of (render_process_id, render_frame_id) associated with this context.
  typedef std::set<std::pair<int, int>> RenderIdSet;
  RenderIdSet render_id_set_;

  // Set of frame_tree_node_id associated with this context. Keeping this list
  // is necessary because, when navigating the main frame, a new (pre-commit)
  // network request will be created before the RenderFrameHost. Consequently we
  // can't rely on valid render IDs. See https://crbug.com/776884 for
  // background.
  typedef std::set<int> NodeIdSet;
  NodeIdSet node_id_set_;

#if DCHECK_IS_ON()
  bool is_shutdown_ = false;
#endif

  Getter getter_;
  base::WeakPtrFactory<CefBrowserContext> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CefBrowserContext);
};

#endif  // CEF_LIBCEF_BROWSER_BROWSER_CONTEXT_IMPL_H_
