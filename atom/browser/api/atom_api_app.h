// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_APP_H_
#define ATOM_BROWSER_API_ATOM_API_APP_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "atom/browser/api/event_emitter.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/browser.h"
#include "atom/browser/browser_observer.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/promise_util.h"
#include "base/process/process_iterator.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/icon_manager.h"
#include "chrome/browser/process_singleton.h"
#include "content/public/browser/browser_child_process_observer.h"
#include "content/public/browser/gpu_data_manager_observer.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "net/base/completion_callback.h"
#include "net/ssl/client_cert_identity.h"

#if defined(USE_NSS_CERTS)
#include "chrome/browser/certificate_manager_model.h"
#endif

namespace base {
class FilePath;
}

namespace mate {
class Arguments;
}  // namespace mate

namespace atom {

#if defined(OS_WIN)
enum class JumpListResult : int;
#endif

struct ProcessMetric {
  int type;
  base::ProcessId pid;
  std::unique_ptr<base::ProcessMetrics> metrics;

  ProcessMetric(int type,
                base::ProcessId pid,
                std::unique_ptr<base::ProcessMetrics> metrics);
  ~ProcessMetric();
};

namespace api {

class App : public AtomBrowserClient::Delegate,
            public mate::EventEmitter<App>,
            public BrowserObserver,
            public content::GpuDataManagerObserver,
            public content::BrowserChildProcessObserver {
 public:
  using FileIconCallback =
      base::Callback<void(v8::Local<v8::Value>, const gfx::Image&)>;

  static mate::Handle<App> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

#if defined(USE_NSS_CERTS)
  void OnCertificateManagerModelCreated(
      std::unique_ptr<base::DictionaryValue> options,
      const net::CompletionCallback& callback,
      std::unique_ptr<CertificateManagerModel> model);
#endif

  base::FilePath GetAppPath() const;
  void RenderProcessReady(content::RenderProcessHost* host);
  void RenderProcessDisconnected(base::ProcessId host_pid);
  void PreMainMessageLoopRun();

 protected:
  explicit App(v8::Isolate* isolate);
  ~App() override;

  // BrowserObserver:
  void OnBeforeQuit(bool* prevent_default) override;
  void OnWillQuit(bool* prevent_default) override;
  void OnWindowAllClosed() override;
  void OnQuit() override;
  void OnOpenFile(bool* prevent_default, const std::string& file_path) override;
  void OnOpenURL(const std::string& url) override;
  void OnActivate(bool has_visible_windows) override;
  void OnWillFinishLaunching() override;
  void OnFinishLaunching(const base::DictionaryValue& launch_info) override;
  void OnLogin(scoped_refptr<LoginHandler> login_handler,
               const base::DictionaryValue& request_details) override;
  void OnAccessibilitySupportChanged() override;
  void OnPreMainMessageLoopRun() override;
#if defined(OS_MACOSX)
  void OnWillContinueUserActivity(bool* prevent_default,
                                  const std::string& type) override;
  void OnDidFailToContinueUserActivity(const std::string& type,
                                       const std::string& error) override;
  void OnContinueUserActivity(bool* prevent_default,
                              const std::string& type,
                              const base::DictionaryValue& user_info) override;
  void OnUserActivityWasContinued(
      const std::string& type,
      const base::DictionaryValue& user_info) override;
  void OnUpdateUserActivityState(
      bool* prevent_default,
      const std::string& type,
      const base::DictionaryValue& user_info) override;
  void OnNewWindowForTab() override;
#endif

  // content::ContentBrowserClient:
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(content::CertificateRequestResultType)>&
          callback) override;
  void SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList client_certs,
      std::unique_ptr<content::ClientCertificateDelegate> delegate) override;
  bool CanCreateWindow(content::RenderFrameHost* opener,
                       const GURL& opener_url,
                       const GURL& opener_top_level_frame_url,
                       const GURL& source_origin,
                       content::mojom::WindowContainerType container_type,
                       const GURL& target_url,
                       const content::Referrer& referrer,
                       const std::string& frame_name,
                       WindowOpenDisposition disposition,
                       const blink::mojom::WindowFeatures& features,
                       const std::vector<std::string>& additional_features,
                       const scoped_refptr<network::ResourceRequestBody>& body,
                       bool user_gesture,
                       bool opener_suppressed,
                       bool* no_javascript_access) override;

  // content::GpuDataManagerObserver:
  void OnGpuProcessCrashed(base::TerminationStatus status) override;

  // content::BrowserChildProcessObserver:
  void BrowserChildProcessLaunchedAndConnected(
      const content::ChildProcessData& data) override;
  void BrowserChildProcessHostDisconnected(
      const content::ChildProcessData& data) override;
  void BrowserChildProcessCrashed(
      const content::ChildProcessData& data,
      const content::ChildProcessTerminationInfo& info) override;
  void BrowserChildProcessKilled(
      const content::ChildProcessData& data,
      const content::ChildProcessTerminationInfo& info) override;

 private:
  void SetAppPath(const base::FilePath& app_path);
  void ChildProcessLaunched(int process_type, base::ProcessHandle handle);
  void ChildProcessDisconnected(base::ProcessId pid);

  // Get/Set the pre-defined path in PathService.
  base::FilePath GetPath(mate::Arguments* args, const std::string& name);
  void SetPath(mate::Arguments* args,
               const std::string& name,
               const base::FilePath& path);

  void SetDesktopName(const std::string& desktop_name);
  std::string GetLocale();
  std::string GetRegion();
  void OnSecondInstance(const base::CommandLine::StringVector& cmd,
                        const base::FilePath& cwd);
  bool HasSingleInstanceLock() const;
  bool RequestSingleInstanceLock();
  void ReleaseSingleInstanceLock();
  bool Relaunch(mate::Arguments* args);
  void DisableHardwareAcceleration(mate::Arguments* args);
  void DisableDomainBlockingFor3DAPIs(mate::Arguments* args);
  bool IsAccessibilitySupportEnabled();
  void SetAccessibilitySupportEnabled(bool enabled);
  Browser::LoginItemSettings GetLoginItemSettings(mate::Arguments* args);
#if defined(USE_NSS_CERTS)
  void ImportCertificate(const base::DictionaryValue& options,
                         const net::CompletionCallback& callback);
#endif
  void GetFileIcon(const base::FilePath& path, mate::Arguments* args);

  std::vector<mate::Dictionary> GetAppMetrics(v8::Isolate* isolate);
  v8::Local<v8::Value> GetGPUFeatureStatus(v8::Isolate* isolate);
  v8::Local<v8::Promise> GetGPUInfo(v8::Isolate* isolate,
                                    const std::string& info_type);
  void EnableSandbox(mate::Arguments* args);
  void EnableMixedSandbox(mate::Arguments* args);

#if defined(OS_MACOSX)
  bool MoveToApplicationsFolder(mate::Arguments* args);
  bool IsInApplicationsFolder();
#endif
#if defined(MAS_BUILD)
  base::Callback<void()> StartAccessingSecurityScopedResource(
      mate::Arguments* args);
#endif

#if defined(OS_WIN)
  // Get the current Jump List settings.
  v8::Local<v8::Value> GetJumpListSettings();

  // Set or remove a custom Jump List for the application.
  JumpListResult SetJumpList(v8::Local<v8::Value> val, mate::Arguments* args);
#endif  // defined(OS_WIN)

  std::unique_ptr<ProcessSingleton> process_singleton_;

#if defined(USE_NSS_CERTS)
  std::unique_ptr<CertificateManagerModel> certificate_manager_model_;
#endif

  // Tracks tasks requesting file icons.
  base::CancelableTaskTracker cancelable_task_tracker_;

  base::FilePath app_path_;

  using ProcessMetricMap =
      std::unordered_map<base::ProcessId, std::unique_ptr<atom::ProcessMetric>>;
  ProcessMetricMap app_metrics_;

  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_APP_H_
