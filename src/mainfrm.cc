#include "mainfrm.h"

#include <wx/artprov.h>
#include <wx/imagpng.h>
#include <wx/stdpaths.h>

#include "window_state_manager.h"

#include "global_executor.h"
#include "options.h"
#include "oss_site_config.h"
#include "site_manager_dialog.h"
#include "storage.h"
#include "sync_task_dialog.h"
#include "theme_provider.h"
#include "utils.h"

namespace {
enum {
    opID_SITE_MANAGER = wxID_HIGHEST + 1,
    opID_SCHEDULE_MANAGER,
    opID_ADD_SYNC,
    opID_VIEW_TWINMODE,
    opID_VIEW_TASKLIST,
    opID_CLOSE_ALL,
    opID_SPLITTER_TOP,
    opID_DIFF,
};
}

// clang-format off
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_CLOSE(MainFrame::OnClose)
EVT_MENU(wxID_ANY, MainFrame::OnMenuHandler)
EVT_MENU(opID_SITE_MANAGER, MainFrame::OnSiteManager)
EVT_MENU(opID_VIEW_TASKLIST, MainFrame::OnViewTaskList)
EVT_MENU(opID_CLOSE_ALL, MainFrame::OnCloseAll)
EVT_SPLITTER_UNSPLIT(opID_SPLITTER_TOP, MainFrame::OnSplitterTopUnsplit)
EVT_TOOL(wxID_UP, MainFrame::OnUp)
EVT_TOOL(wxID_BACKWARD, MainFrame::OnBackward)
EVT_TOOL(wxID_FORWARD, MainFrame::OnForward)
EVT_TOOL(wxID_REFRESH, MainFrame::OnRefresh)
EVT_MENU(opID_VIEW_TWINMODE, MainFrame::OnTwinMode)
EVT_MENU(opID_DIFF, MainFrame::OnDiff)
EVT_MENU(opID_SCHEDULE_MANAGER, MainFrame::OnSyncManager)
EVT_MENU(opID_ADD_SYNC, MainFrame::OnAddSync)
EVT_MENU(wxID_COPY, MainFrame::OnCopy)
EVT_SIZE(MainFrame::OnSize)
wxEND_EVENT_TABLE();
// clang-format on

#ifdef __WXMAC__
void fix_macos_window_style(wxFrame *frame);
#endif

MainFrame::MainFrame() {
    wxMenuBar *menuBar = new wxMenuBar;

    wxRect screenSize = WindowStateManager::GetScreenDimension();

    wxSize initialSize;
    initialSize.x = std::min(1200, screenSize.GetWidth() - 10);
    initialSize.y = std::min(900, screenSize.GetHeight() - 150);

    Create(nullptr, wxID_ANY, _T("Osspan"), wxDefaultPosition, initialSize);

    SetSizeHints(700, 500);

    wxMenu *menuFile = new wxMenu;
    menuFile->Append(opID_SITE_MANAGER, _("Site Manager"));
    menuFile->Append(opID_SCHEDULE_MANAGER, _("Task Manager"));
    menuFile->AppendSeparator();
    menuFile->Append(opID_CLOSE_ALL, _("Close All"));
    menuFile->Append(wxID_EXIT);
    menuBar->Append(menuFile, _("File"));

    wxMenu *menuView = new wxMenu;
    menuView->Append(opID_VIEW_TWINMODE,
                     _("Local Remote Split"),
                     wxEmptyString,
                     wxITEM_CHECK)
            ->Check();
    menuView->Append(opID_VIEW_TASKLIST,
                     _("Explorer Task Split"),
                     wxEmptyString,
                     wxITEM_CHECK)
            ->Check();
    menuBar->Append(menuView, _("View"));

    SetMenuBar(menuBar);

    wxSize iconSize = themeProvider()->GetIconSize(iconSizeSmall, true);
#ifdef __WXMAC__
    if (iconSize.GetWidth() >= 32) {
        iconSize = wxSize(32, 32);
    } else {
        iconSize = wxSize(24, 24);
    }
#endif

    wxImage::AddHandler(new wxPNGHandler);

    toolBar_ = CreateToolBar();

    toolBar_->AddTool(
            wxID_UP,
            _("Up"),
            themeProvider()->CreateBitmap("ART_UP", wxART_OTHER, iconSize),
            wxBitmap(),
            wxITEM_NORMAL,
            _("Up"));
    toolBar_->AddTool(wxID_BACKWARD,
                      _("Back"),
                      themeProvider()->CreateBitmap(
                              "ART_BACKWARD", wxART_OTHER, iconSize),
                      wxBitmap(),
                      wxITEM_NORMAL,
                      _("Back"));
    toolBar_->AddTool(
            wxID_FORWARD,
            _("Forward"),
            themeProvider()->CreateBitmap("ART_FORWARD", wxART_OTHER, iconSize),
            wxBitmap(),
            wxITEM_NORMAL,
            _("Forward"));
    toolBar_->AddTool(
            wxID_REFRESH,
            _("Refresh"),
            themeProvider()->CreateBitmap("ART_REFRESH", wxART_OTHER, iconSize),
            wxBitmap(),
            wxITEM_NORMAL,
            _("Refresh"));
    toolBar_->AddTool(
            wxID_COPY,
            _("Copy"),
            themeProvider()->CreateBitmap("ART_COPY", wxART_OTHER, iconSize),
            wxBitmap(),
            wxITEM_NORMAL,
            _("Copy"));
    toolBar_->AddTool(
            opID_DIFF,
            _("Diff"),
            themeProvider()->CreateBitmap("ART_COMPARE", wxART_OTHER, iconSize),
            wxBitmap(),
            wxITEM_CHECK,
            _("Diff"));
    toolBar_->AddTool(
            opID_ADD_SYNC,
            _("Sync"),
            themeProvider()->CreateBitmap("ART_SYNC", wxART_OTHER, iconSize),
            wxBitmap(),
            wxITEM_NORMAL,
            _("Sync"));
    toolBar_->Realize();

    statusBar_ = new StatusBar(this);
    SetStatusBar(statusBar_);

    (void)scheduleList();

    taskList()->Attach(this);

    splitterTop_ = new wxSplitterWindow(this,
                                        opID_SPLITTER_TOP,
                                        wxDefaultPosition,
                                        wxDefaultSize,
                                        wxSP_LIVE_UPDATE);
    splitterTop_->SetSashGravity(0.7);
    splitterTop_->SetSize(GetClientSize());

    explorer_ = new MultiTabExplorer(splitterTop_);
    explorer_->AddListener(this);

    taskList()->Load();
    int splitopt = options().get_int(OPTION_SPLITTERTOP_SPLIT);
    bool split = splitopt == 0   ? !taskList()->root()->children.empty()
                 : splitopt == 1 ? true
                                 : false;
    if (split) {
        taskListCtrl_ = new TaskListCtrl(splitterTop_);
        splitterTop_->SplitHorizontally(explorer_, taskListCtrl_);
    } else {
        splitterTop_->Initialize(explorer_);
        explorer_->SetSize(splitterTop_->GetClientSize());
        menuBar->Check(opID_VIEW_TASKLIST, false);
    }

    SetupKeyboardAccelerators();

#ifdef __WXMAC__
    fix_macos_window_style(this);
#endif

    windowStateManager_.reset(new WindowStateManager(this));
    windowStateManager_->Restore();

    if (split) {
        RestoreSplitterPosition();
    }

    const auto &siteNode = ossSiteConfig()->sites.front();
    const std::vector<std::string> &lastOpenSites =
            options().get_string_vec(OPTION_LASTOPEN_SITES);
    if (lastOpenSites.empty()) {
        wxTheApp->CallAfter([this]() { OpenSiteManager(); });
    } else {
        for (const auto &name : lastOpenSites) {
            explorer_->Open(name);
        }
        int lastOpenIndex = options().get_int(OPTION_LASTOPEN_INDEX);
        if (lastOpenIndex >= 0) {
            explorer_->SetSelection(lastOpenIndex);
        }
    }
}

MainFrame::~MainFrame() {
}

void MainFrame::OnClose(wxCloseEvent &event) {
    if (windowStateManager_) {
        windowStateManager_->Remember();
    }
    RememberSplitterPosition();

    ossSiteConfig()->Save();

    std::vector<std::string> openSites = explorer_->GetOpenedSites();
    int selection = explorer_->GetSelection();
    options().set(OPTION_LASTOPEN_SITES, openSites);
    options().set(OPTION_LASTOPEN_INDEX, selection);

    taskList()->Close();
    globalExecutor()->cancel();

    options().Save();
}

static int tab_hotkey_ids[10];

std::map<int, std::pair<std::function<void(wxTextEntry *)>, wxChar>>
        keyboardCommands;

#ifdef __WXMAC__
wxTextEntry *GetSpecialTextEntry(wxWindow *w, wxChar cmd) {
    if (cmd == 'A' || cmd == 'V') {
        wxTextCtrl *text = dynamic_cast<wxTextCtrl *>(w);
        if (text) {
            return text;
        }
    }
    return dynamic_cast<wxComboBox *>(w);
}
#else
wxTextEntry *GetSpecialTextEntry(wxWindow *, wxChar) {
    return 0;
}
#endif

bool HandleKeyboardCommand(wxCommandEvent &event, wxWindow &parent) {
    auto const &it = keyboardCommands.find(event.GetId());
    if (it == keyboardCommands.end()) {
        return false;
    }

    wxTextEntry *e = GetSpecialTextEntry(parent.FindFocus(), it->second.second);
    if (e) {
        it->second.first(e);
    } else {
        event.Skip();
    }
    return true;
}

void MainFrame::SetupKeyboardAccelerators() {
    std::vector<wxAcceleratorEntry> entries;
    for (int i = 0; i < 10; i++) {
        tab_hotkey_ids[i] = wxNewId();
        entries.emplace_back(wxACCEL_CMD, (int)'0' + i, tab_hotkey_ids[i]);
        entries.emplace_back(wxACCEL_ALT, (int)'0' + i, tab_hotkey_ids[i]);
    }

#ifdef __WXMAC__
    keyboardCommands[wxNewId()] =
            std::make_pair([](wxTextEntry *e) { e->Cut(); }, 'X');
    keyboardCommands[wxNewId()] =
            std::make_pair([](wxTextEntry *e) { e->Copy(); }, 'C');
    keyboardCommands[wxNewId()] =
            std::make_pair([](wxTextEntry *e) { e->Paste(); }, 'V');
    keyboardCommands[wxNewId()] =
            std::make_pair([](wxTextEntry *e) { e->SelectAll(); }, 'A');
    for (const auto &command : keyboardCommands) {
        entries.emplace_back(wxACCEL_CMD, command.second.second, command.first);
    }

    // Ctrl+(Shift)+Tab to switch between tabs
    int id = wxNewId();
    entries.emplace_back(wxACCEL_RAW_CTRL, '\t', id);
    Bind(
            wxEVT_MENU, [this](wxEvent &) { explorer_->AdvanceTab(true); }, id);
    id = wxNewId();
    entries.emplace_back(wxACCEL_RAW_CTRL | wxACCEL_SHIFT, '\t', id);
    Bind(
            wxEVT_MENU,
            [this](wxEvent &) { explorer_->AdvanceTab(false); },
            id);

#endif
    wxAcceleratorTable accel(entries.size(), &entries[0]);
    SetAcceleratorTable(accel);
}

void MainFrame::OnMenuHandler(wxCommandEvent &event) {
    if (event.GetId() == wxID_EXIT) {
        Close();
    } else if (HandleKeyboardCommand(event, *this)) {
        return;
    } else if (HandleTabSelection(event)) {
        return;
    }

    event.Skip();
}

void MainFrame::OpenSiteManager() {
    SiteManagerDialog *smDlg = new SiteManagerDialog(this, wxID_ANY);
    if (smDlg->ShowModal() == wxID_OK) {
        OssSiteNode *siteNode = smDlg->GetSiteNode();
        explorer_->Open(siteNode->name);
    }
    smDlg->Destroy();
}

bool MainFrame::HandleTabSelection(wxCommandEvent &event) {
    for (int i = 0; i < 10; i++) {
        if (event.GetId() != tab_hotkey_ids[i]) {
            continue;
        }
        size_t tabCount = explorer_->GetTabCount();
        if (tabCount > 1 && i <= tabCount) {
            int sel = i - 1;
            if (sel < 0) {
                sel = 9;
            }
            explorer_->SetSelection(sel);
            return true;
        }
    }
    return false;
}

void MainFrame::OnSiteManager(wxCommandEvent &event) {
    OpenSiteManager();
}

void MainFrame::OnViewTaskList(wxCommandEvent &event) {
    if (splitterTop_->IsSplit()) {
        int pos = splitterTop_->GetSashPosition();
        options().set(OPTION_SPLITTERTOP_SPLIT, -1);
        options().set(OPTION_SPLITTERTOP_POSITION, pos);
        splitterTop_->Unsplit(taskListCtrl_);
    } else {
        options().set(OPTION_SPLITTERTOP_SPLIT, 1);
        if (!taskListCtrl_) {
            taskListCtrl_ = new TaskListCtrl(splitterTop_);
        }
        splitterTop_->SplitHorizontally(explorer_, taskListCtrl_);
        RestoreSplitterPosition();
    }
}

void MainFrame::OnCloseAll(wxCommandEvent &event) {
    explorer_->Clear();
}

void MainFrame::OnSplitterTopUnsplit(wxSplitterEvent &event) {
    options().set(OPTION_SPLITTERTOP_SPLIT, -1);
    GetMenuBar()->Check(opID_VIEW_TASKLIST, false);
}

void MainFrame::ExplorerSwitched(Explorer *explorer) {
    SetLabel("Osspan - " + explorer->siteName());
    toolBar_->ToggleTool(opID_DIFF, explorer->InDiffMode());
}

void MainFrame::OnUp(wxCommandEvent &event) {
    explorer_->Up();
}

void MainFrame::OnBackward(wxCommandEvent &event) {
    explorer_->Backward();
}

void MainFrame::OnForward(wxCommandEvent &event) {
    explorer_->Forward();
}

void MainFrame::OnRefresh(wxCommandEvent &event) {
    explorer_->RefreshFileItems();
}

void MainFrame::OnTwinMode(wxCommandEvent &event) {
    explorer_->OnTwinMode();
}

void MainFrame::OnDiff(wxCommandEvent &event) {
    explorer_->OnDiff();
}

void MainFrame::OnSyncManager(wxCommandEvent &event) {
    OpenSyncManagerDialog(true);
}

void MainFrame::OnAddSync(wxCommandEvent &event) {
    OpenSyncManagerDialog(false);
}

void MainFrame::OpenSyncManagerDialog(bool expandList) {
    SyncTaskDialog *dlg = new SyncTaskDialog(this, wxID_ANY);
    dlg->ExpandScheduleList(expandList);
    std::shared_ptr<LocalSite> localSite = explorer_->GetCurrentLocalSite();
    std::shared_ptr<OssSite> ossSite = explorer_->GetCurrentOssSite();
    if (localSite) {
        dlg->SetLocalPath(localSite->GetCurrentPath());
    }
    if (ossSite) {
        dlg->SetOssPath(ossSite->name(), ossSite->GetCurrentPath());
    }
    if (dlg->ShowModal()) {
    }
    dlg->Destroy();
}

void MainFrame::OnCopy(wxCommandEvent &event) {
    explorer_->Copy();
}

void MainFrame::TaskAdded(const TaskPtr &parent, const TaskPtr &task) {
    int splitopt = options().get_int(OPTION_SPLITTERTOP_SPLIT);
    if (splitopt >= 0) {
        if (!splitterTop_->IsSplit()) {
            GetMenuBar()->Check(opID_VIEW_TASKLIST, true);
            if (!taskListCtrl_) {
                taskListCtrl_ = new TaskListCtrl(splitterTop_);
            }
            splitterTop_->SplitHorizontally(explorer_, taskListCtrl_);
        }
    }
}

void MainFrame::ExplorerDiffQuit(Explorer *explorer) {
    if (explorer == explorer_->GetActiveExplorer()) {
        toolBar_->ToggleTool(opID_DIFF, false);
    }
}

void MainFrame::ExplorerSiteUpdated(Explorer *explorer, Site *site) {
    if (explorer == explorer_->GetActiveExplorer() &&
        site == explorer->GetActiveSite()) {
        SetDirStatusText(explorer);
    }
}

void MainFrame::ExplorerSiteSwitched(Explorer *explorer, Site *site) {
    if (explorer == explorer_->GetActiveExplorer()) {
        SetDirStatusText(explorer);
    }
}

void MainFrame::ExplorerSelectionUpdated(Explorer *explorer,
                                         FileListCtrl *fileListCtrl) {
    if (explorer == explorer_->GetActiveExplorer() &&
        fileListCtrl == explorer->GetActiveListCtrl()) {
        SetDirStatusText(explorer);
    }
}

void MainFrame::TwinModeSwitched(Explorer *explorer, bool twinMode) {
    GetMenuBar()->Check(opID_VIEW_TWINMODE, twinMode);
}

void MainFrame::SetDirStatusText(Explorer *explorer) {
    FileListCtrl *fileListCtrl = explorer->GetActiveListCtrl();
    int selectedDirCount = fileListCtrl->GetSelectedDirCount();
    int selectedFileCount = fileListCtrl->GetSelectedFileCount();
    if (selectedDirCount || selectedFileCount) {
        if (!selectedFileCount) {
            statusBar_->SetStatusText(wxString::Format(
                    _("selected %d directory"), selectedDirCount));
        } else if (!selectedDirCount) {
            statusBar_->SetStatusText(wxString::Format(
                    _("selected %d file total %s Bytes"),
                    selectedFileCount,
                    wxString::Format(_T("%'d"),
                                     fileListCtrl->GetSelectedTotalSize())));
        } else {
            statusBar_->SetStatusText(wxString::Format(
                    _("selected %d directory %d file total %s bytes"),
                    selectedDirCount,
                    selectedFileCount,
                    wxString::Format(_T("%'d"),
                                     fileListCtrl->GetSelectedTotalSize())));
        }
    } else {
        const DirPtr &dir = fileListCtrl->GetSite()->GetCurrentDir();
        if (dir->dirCount == 0 && dir->fileCount == 0) {
            statusBar_->SetStatusText(_("Empty Directory"));
        } else if (dir->fileCount == 0) {
            statusBar_->SetStatusText(
                    wxString::Format(_("%d directory"), dir->dirCount));
        } else if (dir->dirCount == 0) {
            statusBar_->SetStatusText(wxString::Format(
                    _("%d file total %s Bytes"),
                    dir->fileCount,
                    wxString::Format(_T("%'d"), dir->totalSize)));
        } else {
            statusBar_->SetStatusText(wxString::Format(
                    _("%d directory %d file total %s bytes"),
                    dir->dirCount,
                    dir->fileCount,
                    wxString::Format(_T("%'d"), dir->totalSize)));
        }
    }
}

void MainFrame::RestoreSplitterPosition() {
    int pos = options().get_int(OPTION_SPLITTERTOP_POSITION);
    if (pos == 0) {
        int height = GetClientSize().GetHeight();
        splitterTop_->SetSashPosition(height * 0.7);
    } else {
        splitterTop_->SetSashPosition(pos);
    }
}

void MainFrame::RememberSplitterPosition() {
    if (splitterTop_->IsSplit()) {
        int pos = splitterTop_->GetSashPosition();
    }
}

void MainFrame::OnSize(wxSizeEvent &event) {
    event.Skip();
}
