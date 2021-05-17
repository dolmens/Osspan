#include "site_manager_dialog.h"
#include "oss_regions.h"

#include <deque>
#include <set>

#ifndef __WXMAC__
#define DEFAULT_TREE_STYLE wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT
#else
#define DEFAULT_TREE_STYLE wxTR_HAS_BUTTONS | wxTR_NO_LINES
#endif

// clang-format off
wxBEGIN_EVENT_TABLE(SiteManagerDialog, wxDialogEx)
EVT_TREE_SEL_CHANGED(wxID_ANY, SiteManagerDialog::OnSelChanged)
EVT_TREE_ITEM_ACTIVATED(wxID_ANY, SiteManagerDialog::OnItemActivated)
EVT_TREE_ITEM_MENU(wxID_ANY, SiteManagerDialog::OnTreeItemMenu)
EVT_MENU(wxID_NEW, SiteManagerDialog::OnNewItem)
EVT_MENU(wxID_DELETE, SiteManagerDialog::OnDeleteItem)
EVT_BUTTON(wxID_OK, SiteManagerDialog::OnOk)
wxEND_EVENT_TABLE();
// clang-format on

SiteManagerDialog::SiteManagerDialog(wxWindow *parent, wxWindowID id)
    : wxDialogEx(parent,
                 id,
                 _("OSS Site Manager"),
                 wxDefaultPosition,
                 wxDefaultSize,
                 wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER | wxCLOSE_BOX) {
    const auto &lay = layout();
    auto main = lay.createMain(this, 1);
    main->AddGrowableCol(0);
    main->AddGrowableRow(1);

    main->Add(new wxStaticText(this, wxID_ANY, _("Select Entry:")));

    wxBoxSizer *sides = new wxBoxSizer(wxHORIZONTAL);
    main->Add(sides, lay.grow)->SetProportion(1);

    siteTree_ = new wxTreeCtrl(this,
                               wxID_ANY,
                               wxDefaultPosition,
                               wxDefaultSize,
                               DEFAULT_TREE_STYLE | wxBORDER_SUNKEN);

    BuildSiteTree();
    sides->Add(siteTree_, 1, wxEXPAND);
    sides->AddSpacer(lay.gap);

    auto form = lay.createFlex(2);
    sides->Add(form, 1, wxEXPAND);

    form->AddGrowableCol(1);

    form->Add(new wxStaticText(
            this, wxID_ANY, _("Name"), wxDefaultPosition, wxSize(60, -1)));
    name_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition);
    form->Add(name_, 1, wxEXPAND);

    form->Add(new wxStaticText(
            this, wxID_ANY, _("Region"), wxDefaultPosition, wxSize(60, -1)));

    const std::vector<const char *> &regions = ossRegions();
    wxArrayString regionOptions(regions.size(), (const char **)regions.data());
    regionOptions[0] = _("Auto Select");
    region_ = new wxComboBox(this,
                             wxID_ANY,
                             wxEmptyString,
                             wxDefaultPosition,
                             wxDefaultSize,
                             regionOptions,
                             wxCB_READONLY);
    form->Add(region_, 1, wxEXPAND);

    form->Add(new wxStaticText(
            this, wxID_ANY, _T("KeyId"), wxDefaultPosition, wxSize(60, -1)));
    keyId_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(keyId_, 1, wxEXPAND);

    form->Add(new wxStaticText(
            this, wxID_ANY, _T("KeyScret"), wxDefaultPosition, wxSize(60, -1)));
    keySecret_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(keySecret_, 1, wxEXPAND);

    EnableSiteEditors(false);

    wxSizer *buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    main->Add(buttonSizer, 1, wxEXPAND);

    SetClientSize(wxSize(768, 360));

    if (ossSiteConfig()->sites.empty()) {
        AddItem();
    }
}

class SiteManagerItemData : public wxTreeItemData {
public:
    SiteManagerItemData(OssSiteNode *siteNode) : siteNode(siteNode) {}

    OssSiteNode *siteNode;
};

void SiteManagerDialog::BuildSiteTree() {
    auto root = siteTree_->AddRoot(_("OSS Site List"));
    for (const auto &siteNode : ossSiteConfig()->sites) {
        siteTree_->AppendItem(root,
                              siteNode->name,
                              -1,
                              -1,
                              new SiteManagerItemData(siteNode.get()));
    }
    siteTree_->Expand(root);
}

void SiteManagerDialog::OnSelChanged(wxTreeEvent &event) {
    wxTreeItemId itemId = event.GetItem();
    if (itemId == siteTree_->GetRootItem()) {
        EnableSiteEditors(false);
    } else {
        EnableSiteEditors(true);
        siteNode_ = ((SiteManagerItemData *)siteTree_->GetItemData(itemId))
                            ->siteNode;
        name_->SetValue(siteNode_->name);
        if (siteNode_->region.empty()) {
            region_->SetSelection(0);
        } else {
            const std::vector<const char *> &regionCodes = ossRegionCodes();
            for (size_t i = 0; i < regionCodes.size(); i++) {
                if (siteNode_->region == regionCodes[i]) {
                    region_->SetSelection(i);
                    break;
                }
            }
        }
        keyId_->SetValue(siteNode_->keyId);
        keySecret_->SetValue(siteNode_->keySecret);
    }
}

void SiteManagerDialog::OnItemActivated(wxTreeEvent &event) {
    wxTreeItemId itemId = siteTree_->GetSelection();
    if (itemId != siteTree_->GetRootItem()) {
        Ok();
    }
}

void SiteManagerDialog::OnTreeItemMenu(wxTreeEvent &event) {
    wxTreeItemId itemId = event.GetItem();
    wxPoint clientpt = event.GetPoint();
    wxPoint screenpt = ClientToScreen(clientpt);
    wxMenu menu;
    menu.Append(wxID_NEW, _("New"));
    menu.Append(wxID_DELETE, _("Delete"));

    PopupMenu(&menu, clientpt);
}

void SiteManagerDialog::OnNewItem(wxCommandEvent &event) {
    AddItem();
}

void SiteManagerDialog::AddItem() {
    OssSiteNodePtr ossSiteNode = ossSiteConfig()->Add();
    wxTreeItemId item =
            siteTree_->AppendItem(siteTree_->GetRootItem(),
                                  ossSiteNode->name,
                                  -1,
                                  -1,
                                  new SiteManagerItemData(ossSiteNode.get()));
    siteTree_->SelectItem(item);
}

void SiteManagerDialog::OnDeleteItem(wxCommandEvent &event) {
    wxTreeItemId item = siteTree_->GetSelection();
    if (item.IsOk()) {
        OssSiteNode *ossSiteNode =
                ((SiteManagerItemData *)siteTree_->GetItemData(item))->siteNode;
        siteTree_->Delete(item);
        ossSiteConfig()->Remove(ossSiteNode);
    }
}

void SiteManagerDialog::OnOk(wxCommandEvent &event) {
    Ok();
}

void SiteManagerDialog::Ok() {
    wxString msg;
    if (name_->IsEmpty()) {
        msg = _("Please input name");
    } else if (keyId_->IsEmpty()) {
        msg = _("Please input key id");
    } else if (keySecret_->IsEmpty()) {
        msg = _("Please input key secret");
    }
    if (!msg.IsEmpty()) {
        wxMessageBox(msg, _("Error"));
        return;
    }
    bool updated = false;
    std::string name = name_->GetValue().ToStdString();
    if (siteNode_->name != name) {
        if (ossSiteConfig()->NameExists(siteNode_, name)) {
            wxMessageBox(_("Name Exists!"), _("Error"));
            return;
        }
        siteNode_->name = std::move(name);
        updated = true;
    }
    const std::vector<const char *> &regionCodes = ossRegionCodes();
    std::string region = regionCodes[region_->GetSelection()];
    if (siteNode_->region != region) {
        siteNode_->region = std::move(region);
        updated = true;
    }
    std::string keyId = keyId_->GetValue().ToStdString();
    if (siteNode_->keyId != keyId) {
        siteNode_->keyId = std::move(keyId);
        updated = true;
    }
    std::string keySecret = keySecret_->GetValue().ToStdString();
    if (siteNode_->keySecret != keySecret) {
        siteNode_->keySecret = std::move(keySecret);
        updated = true;
    }
    if (updated) {
        ossSiteConfig()->Update(siteNode_);
    }
    EndModal(wxID_OK);
}

void SiteManagerDialog::EnableSiteEditors(bool enable) {
    name_->Enable(enable);
    region_->Enable(enable);
    keyId_->Enable(enable);
    keySecret_->Enable(enable);
}
