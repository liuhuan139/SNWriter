#pragma once

#include <gtkmm.h>

#include <cairomm/context.h>
#include <string>
#include <utility>

class MainWindow : public Gtk::Window {
public:
    explicit MainWindow(const std::string& executable_path);
    ~MainWindow() override;

private:
    enum class WorkerKind { Patch, WriteDevice, ExportAndroid };

    bool on_delete_event(GdkEventAny* any_event) override;
    void on_menu_browse();
    void on_menu_quit();
    void on_menu_usage();
    void on_export_android_clicked();
    void on_choose_source_clicked();
    void on_sn_changed();
    void on_patch_clicked();
    void on_write_device_clicked();
    void schedule_toast_hide();
    void update_patch_button_state();
    std::string sanitize_sn(const std::string& text) const;
    void show_toast(const Glib::ustring& text, bool success);
    void set_status_text(const Glib::ustring& text, bool success, bool processing);
    bool on_status_light_draw(const Cairo::RefPtr<Cairo::Context>& cr);
    bool on_status_pulse_tick();
    bool run_command_capture(const std::string& command, std::string* output) const;

    std::string executable_path_;
    std::string source_path_;
    const std::string dest_path_ = "/tmp/snedit/ret/proinfo";

    GPid worker_pid_ = 0;
    guint worker_watch_id_ = 0;
    WorkerKind worker_kind_ = WorkerKind::Patch;
    double pulse_phase_ = 0.0;
    bool is_processing_ = false;
    bool status_is_success_ = true;
    sigc::connection pulse_timer_conn_;

    Gtk::Box root_box_{Gtk::ORIENTATION_VERTICAL, 0};
    Gtk::MenuBar menu_bar_;
    Gtk::MenuItem file_menu_item_{"文件"};
    Gtk::Menu file_menu_;
    Gtk::MenuItem browse_menu_item_{"浏览"};
    Gtk::MenuItem quit_menu_item_{"退出"};

    Gtk::MenuItem tools_menu_item_{"工具"};
    Gtk::Menu tools_menu_;
    Gtk::MenuItem export_android_menu_item_{"从Android导出"};

    Gtk::MenuItem help_menu_item_{"帮助"};
    Gtk::Menu help_menu_;
    Gtk::MenuItem usage_menu_item_{"操作说明"};

    Gtk::Box content_box_{Gtk::ORIENTATION_VERTICAL, 14};
    Gtk::Label title_label_{"SNWriter"};
    Gtk::Grid form_grid_;

    Gtk::Label source_label_{"选择二进制文件:"};
    Gtk::Entry source_entry_;
    Gtk::Button source_button_{"浏览..."};

    Gtk::Label sn_label_{"SN(8位):"};
    Gtk::Entry sn_entry_;

    Gtk::Button patch_button_{"修改SN"};
    Gtk::Button write_device_button_{"写入到设备"};
    Gtk::Box button_row_{Gtk::ORIENTATION_HORIZONTAL, 18};

    Gtk::Revealer toast_revealer_;
    Gtk::Label toast_label_;
    Gtk::Label source_hint_label_{"当前来源: 本地文件"};
    Gtk::Label output_hint_label_{"输出路径: /tmp/snedit/ret/proinfo"};
    Gtk::Box status_bar_{Gtk::ORIENTATION_HORIZONTAL, 8};
    Gtk::Label status_label_{"状态: 就绪"};
    Gtk::DrawingArea status_light_;
};
