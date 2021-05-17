#include "input_dialog.h"

InputDialog::InputDialog(wxWindow *parent,
        wxString const &title,
        wxString const &text,
        int max_len,
        bool password)
    : wxDialogEx(parent, wxID_ANY, title) {
    auto &lay = layout();
    auto *main = lay.createMain(this);
    main->Add(new wxStaticText(this, wxID_ANY, text));
    textCtrl_ = new wxTextCtrl(this,
            wxID_ANY,
            wxString(),
            wxDefaultPosition,
            wxDefaultSize,
            password ? wxTE_PASSWORD : 0);
    main->Add(textCtrl_, lay.grow)->SetMinSize(lay.dlgUnits(150), -1);
    if (max_len != -1) {
        textCtrl_->SetMaxLength(max_len);
    }
    auto *buttons = lay.createButtonSizer(this, main, true);

    auto ok = new wxButton(this, wxID_OK, "OK");
    ok->SetDefault();
    buttons->AddButton(ok);

    auto cancel = new wxButton(this, wxID_CANCEL, "Cancel");
    buttons->AddButton(cancel);
    buttons->Realize();

    auto onButton = [this](wxEvent &evt) { EndModal(evt.GetId()); };
    ok->Bind(wxEVT_BUTTON, onButton);
    cancel->Bind(wxEVT_BUTTON, onButton);

    GetSizer()->Fit(this);

    textCtrl_->SetFocus();
    ok->Disable();

    textCtrl_->Bind(wxEVT_TEXT, [this, ok](const wxEvent &) {
        ok->Enable(allowEmpty_ || !textCtrl_->GetValue().empty());
    });
}

void InputDialog::AllowEmpty(bool allowEmpty) { allowEmpty_ = allowEmpty; }

void InputDialog::SetValue(const wxString &value) {
    textCtrl_->SetValue(value);
}

wxString InputDialog::GetValue() const { return textCtrl_->GetValue(); }
