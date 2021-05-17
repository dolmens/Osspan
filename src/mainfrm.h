#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "defs.h"
#include "multi_tab_explorer.h"
#include "statusbar.h"
#include "task_list_ctrl.h"
#include "window_state_manager.h"

class MainFrame : public wxNavigationEnabled<wxFrame>,
                  TaskListListener,
                  MultiTabExplorerListener {
public:
    MainFrame();
    ~MainFrame();

    StatusBar *statusBar() const { return statusBar_; }

protected:
    void OnClose(wxCloseEvent &event);
    void SetupKeyboardAccelerators();
    void OnMenuHandler(wxCommandEvent &event);
    void OpenSiteManager();
    bool HandleTabSelection(wxCommandEvent &event);
    void OnSiteManager(wxCommandEvent &event);
    void OnViewTaskList(wxCommandEvent &event);
    void OnCloseAll(wxCommandEvent &event);

    void OnSplitterTopUnsplit(wxSplitterEvent &event);

    void OnUp(wxCommandEvent &event);
    void OnBackward(wxCommandEvent &event);
    void OnForward(wxCommandEvent &event);
    void OnRefresh(wxCommandEvent &event);
    void OnTwinMode(wxCommandEvent &event);
    void OnDiff(wxCommandEvent &event);
    void OnSyncManager(wxCommandEvent &event);
    void OnAddSync(wxCommandEvent &event);
    void OnCopy(wxCommandEvent &event);

    void TaskAdded(const TaskPtr &parent, const TaskPtr &task) override;
    void TaskAdded(const TaskPtr &parent, const TaskPtrVec &tasks) override {}
    void TaskUpdated(Task *task) override {}
    void TaskRemoved(Task *task) override {}

    void ExplorerSwitched(Explorer *explorer) override;

    void ExplorerSiteUpdated(Explorer *explorer, Site *site) override;
    void ExplorerSiteSwitched(Explorer *explorer, Site *site) override;
    void ExplorerSelectionUpdated(Explorer *explorer,
                                  FileListCtrl *fileListCtrl) override;
    void ExplorerDiffQuit(Explorer *explorer) override;

    void TwinModeSwitched(Explorer *explorer, bool twinMode) override;

    void SetDirStatusText(Explorer *explorer);

    void RestoreSplitterPosition();
    void RememberSplitterPosition();

    void OnSize(wxSizeEvent &event);

private:
    void OpenSyncManagerDialog(bool expandList);

    wxToolBar *toolBar_;
    wxSplitterWindow *splitterTop_{};
    MultiTabExplorer *explorer_{};
    TaskListCtrl *taskListCtrl_{};

    StatusBar *statusBar_;

    std::shared_ptr<WindowStateManager> windowStateManager_{};

    wxDECLARE_EVENT_TABLEex();
};
