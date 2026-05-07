#include "MainWindow.hpp"

#include <glibmm.h>

#include <cctype>
#include <cmath>
#include <filesystem>
#include <sys/wait.h>
#include <vector>

namespace {
constexpr int kSnLength = 8;
constexpr double kPi = 3.14159265358979323846;
}

MainWindow::MainWindow(const std::string& executable_path)
    : executable_path_(executable_path) {
    set_title("SNWriter");
    set_default_size(560, 430);

    add(root_box_);

    file_menu_item_.set_submenu(file_menu_);
    file_menu_.append(browse_menu_item_);
    file_menu_.append(quit_menu_item_);

    tools_menu_item_.set_submenu(tools_menu_);
    tools_menu_.append(export_android_menu_item_);

    help_menu_item_.set_submenu(help_menu_);
    help_menu_.append(usage_menu_item_);

    menu_bar_.append(file_menu_item_);
    menu_bar_.append(tools_menu_item_);
    menu_bar_.append(help_menu_item_);

    root_box_.pack_start(menu_bar_, Gtk::PACK_SHRINK);

    content_box_.set_margin_top(34);
    content_box_.set_margin_start(32);
    content_box_.set_margin_end(32);
    content_box_.set_margin_bottom(18);
    root_box_.pack_start(content_box_, Gtk::PACK_EXPAND_WIDGET);

    title_label_.set_halign(Gtk::ALIGN_CENTER);
    title_label_.set_name("main-title");
    content_box_.pack_start(title_label_, Gtk::PACK_SHRINK);

    form_grid_.set_column_spacing(10);
    form_grid_.set_row_spacing(14);
    form_grid_.set_hexpand(true);

    source_label_.set_halign(Gtk::ALIGN_START);
    sn_label_.set_halign(Gtk::ALIGN_START);

    source_entry_.set_editable(false);
    sn_entry_.set_max_length(kSnLength);
    source_entry_.set_hexpand(true);
    sn_entry_.set_hexpand(true);
    source_entry_.set_halign(Gtk::ALIGN_FILL);
    sn_entry_.set_halign(Gtk::ALIGN_FILL);

    form_grid_.attach(source_label_, 0, 0, 1, 1);
    form_grid_.attach(source_entry_, 1, 0, 1, 1);
    form_grid_.attach(source_button_, 2, 0, 1, 1);

    form_grid_.attach(sn_label_, 0, 1, 1, 1);
    form_grid_.attach(sn_entry_, 1, 1, 2, 1);

    content_box_.pack_start(form_grid_, Gtk::PACK_SHRINK);

    patch_button_.set_sensitive(false);
    patch_button_.set_halign(Gtk::ALIGN_CENTER);
    patch_button_.set_size_request(220, 44);
    write_device_button_.set_sensitive(false);
    write_device_button_.set_halign(Gtk::ALIGN_CENTER);
    write_device_button_.set_size_request(220, 36);

    button_row_.pack_start(patch_button_, Gtk::PACK_SHRINK);
    button_row_.pack_start(write_device_button_, Gtk::PACK_SHRINK);
    content_box_.pack_start(button_row_, Gtk::PACK_SHRINK);

    source_hint_label_.set_halign(Gtk::ALIGN_START);
    content_box_.pack_start(source_hint_label_, Gtk::PACK_SHRINK);
    // 去除“输出路径”显示，避免占用高度

    toast_revealer_.set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
    toast_revealer_.set_reveal_child(false);
    toast_label_.set_halign(Gtk::ALIGN_CENTER);
    toast_revealer_.add(toast_label_);
    content_box_.pack_start(toast_revealer_, Gtk::PACK_SHRINK);

    status_bar_.set_margin_top(8);
    status_label_.set_halign(Gtk::ALIGN_START);
    status_label_.set_hexpand(true);
    status_bar_.pack_start(status_label_, Gtk::PACK_EXPAND_WIDGET);
    status_light_.set_size_request(14, 14);
    status_bar_.pack_end(status_light_, Gtk::PACK_SHRINK);
    content_box_.pack_end(status_bar_, Gtk::PACK_SHRINK);

    auto css = Gtk::CssProvider::create();
    css->load_from_data(
        "#main-title {"
        "  font-size: 34px;"
        "  font-weight: bold;"
        "  margin-bottom: 20px;"
        "}"
    );
    Gtk::StyleContext::add_provider_for_screen(
        Gdk::Screen::get_default(), css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    browse_menu_item_.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_menu_browse));
    quit_menu_item_.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_menu_quit));
    export_android_menu_item_.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_export_android_clicked));
    usage_menu_item_.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_menu_usage));
    source_button_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_choose_source_clicked));
    patch_button_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_patch_clicked));
    write_device_button_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_write_device_clicked));
    sn_entry_.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_sn_changed));
    status_light_.signal_draw().connect(sigc::mem_fun(*this, &MainWindow::on_status_light_draw));

    show_all_children();
    toast_revealer_.set_reveal_child(false);
    set_status_text("状态: 就绪", true, false);
    pulse_timer_conn_ = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &MainWindow::on_status_pulse_tick), 40);
}

MainWindow::~MainWindow() {
    if (worker_watch_id_ != 0) {
        g_source_remove(worker_watch_id_);
        worker_watch_id_ = 0;
    }
    if (pulse_timer_conn_.connected()) {
        pulse_timer_conn_.disconnect();
    }
}

void MainWindow::on_menu_browse() {
    on_choose_source_clicked();
}

void MainWindow::on_menu_quit() {
    close();
}

void MainWindow::on_menu_usage() {
    Gtk::MessageDialog dialog(
        *this,
        "操作说明",
        false,
        Gtk::MESSAGE_INFO,
        Gtk::BUTTONS_OK,
        true
    );
    dialog.set_secondary_text(
        "1. 点击“浏览...”选择需要修改的二进制文件。\n"
        "2. 输入 8 位 SN（仅数字/字母，字母自动大写）。\n"
        "3. 点击“修改SN”，结果保存到 /tmp/snedit/ret/proinfo。\n"
        "4. 点击“写入到设备”可将结果写回设备 proinfo 分区并自动重启设备。\n\n"
        "Android 导出功能：在“工具 -> 从Android导出”中导出 /dev/block/by-name/proinfo。\n"
        "注意：本工具仅适用于 peridot、calla 等产品。"
    );
    dialog.run();
}

void MainWindow::on_export_android_clicked() {
    if (worker_pid_ != 0) {
        return;
    }

    Gtk::MessageDialog confirm(
        *this,
        "是否从 Android 导出 proinfo 并作为当前源文件？",
        false,
        Gtk::MESSAGE_QUESTION,
        Gtk::BUTTONS_OK_CANCEL,
        true
    );
    confirm.set_secondary_text("将导出到 /tmp/snedit/proinfo.bin");
    if (confirm.run() != Gtk::RESPONSE_OK) {
        show_toast("已取消导出。", false);
        return;
    }

    patch_button_.set_sensitive(false);
    write_device_button_.set_sensitive(false);
    show_toast("正在从 Android 导出...", true);
    set_status_text("状态: 正在导出", true, true);
    worker_kind_ = WorkerKind::ExportAndroid;

    const std::string out_path = "/tmp/snedit/proinfo.bin";
    std::vector<std::string> args{
        executable_path_,
        "--worker",
        "export_android",
        out_path
    };

    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);

    GError* error = nullptr;
    if (!g_spawn_async(
            nullptr,
            argv.data(),
            nullptr,
            static_cast<GSpawnFlags>(G_SPAWN_DO_NOT_REAP_CHILD),
            nullptr,
            nullptr,
            &worker_pid_,
            &error)) {
        const std::string message = error ? error->message : "无法启动后台子进程";
        if (error) {
            g_error_free(error);
        }
        show_toast("启动导出子进程失败: " + message, false);
        set_status_text("状态: 导出失败", false, false);
        update_patch_button_state();
        return;
    }

    worker_watch_id_ = g_child_watch_add(
        worker_pid_,
        [](GPid pid, gint status, gpointer data) {
            auto* self = static_cast<MainWindow*>(data);
            const bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
            if (self->worker_kind_ == WorkerKind::ExportAndroid) {
                if (ok) {
                    const std::string out_path = "/tmp/snedit/proinfo.bin";
                    self->source_path_ = out_path;
                    self->source_entry_.set_text(self->source_path_);
                    self->source_hint_label_.set_text("当前来源: Android 导出的 proinfo");
                    self->show_toast("Android 导出完成，已使用 /tmp/snedit/proinfo.bin", true);
                    self->set_status_text("状态: 导出成功", true, false);
                } else {
                    self->show_toast("adb pull 导出 proinfo 失败，请检查设备连接和分区路径。", false);
                    self->set_status_text("状态: 导出失败", false, false);
                }
            }

            if (self->worker_watch_id_ != 0) {
                g_source_remove(self->worker_watch_id_);
                self->worker_watch_id_ = 0;
            }
            g_spawn_close_pid(pid);
            self->worker_pid_ = 0;
            self->worker_kind_ = WorkerKind::Patch;
            self->update_patch_button_state();
        },
        this
    );
}

void MainWindow::on_choose_source_clicked() {
    Gtk::FileChooserDialog dialog(*this, "选择需要修改的二进制文件", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.add_button("取消", Gtk::RESPONSE_CANCEL);
    dialog.add_button("选择", Gtk::RESPONSE_OK);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        source_path_ = dialog.get_filename();
        source_entry_.set_text(source_path_);
        source_hint_label_.set_text("当前来源: 本地文件");
        // output_hint_label_ 不再显示
        update_patch_button_state();
    }
}

void MainWindow::on_sn_changed() {
    const auto original = sn_entry_.get_text();
    const auto sanitized = sanitize_sn(original);
    if (sanitized != original) {
        sn_entry_.set_text(sanitized);
        sn_entry_.set_position(static_cast<int>(sanitized.size()));
    }
    update_patch_button_state();
}

void MainWindow::on_patch_clicked() {
    const auto sn = sn_entry_.get_text();
    if (sn.size() != kSnLength) {
        show_toast("SN 必须为 8 位字母数字", false);
        return;
    }

    patch_button_.set_sensitive(false);
    write_device_button_.set_sensitive(false);
    show_toast("正在后台执行修改...", true);
    set_status_text("状态: 正在处理", true, true);
    worker_kind_ = WorkerKind::Patch;

    std::vector<std::string> args{
        executable_path_,
        "--worker",
        "patch",
        source_path_,
        dest_path_,
        sn
    };

    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);

    GError* error = nullptr;
    if (!g_spawn_async(
            nullptr,
            argv.data(),
            nullptr,
            static_cast<GSpawnFlags>(G_SPAWN_DO_NOT_REAP_CHILD),
            nullptr,
            nullptr,
            &worker_pid_,
            &error)) {
        const std::string message = error ? error->message : "无法启动后台子进程";
        if (error) {
            g_error_free(error);
        }
        show_toast("启动子进程失败: " + message, false);
        update_patch_button_state();
        return;
    }

    worker_watch_id_ = g_child_watch_add(
        worker_pid_,
        [](GPid pid, gint status, gpointer data) {
            auto* self = static_cast<MainWindow*>(data);
            const bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
            if (self->worker_kind_ == WorkerKind::Patch) {
                if (ok) {
                    self->show_toast("修改完成，已保存到 /tmp/snedit/ret/proinfo", true);
                    self->set_status_text("状态: 处理成功", true, false);
                    self->write_device_button_.set_sensitive(true);
                } else {
                    self->show_toast("修改失败，请检查文件路径和权限。", false);
                    self->set_status_text("状态: 处理失败", false, false);
                    self->write_device_button_.set_sensitive(false);
                }
            } else if (self->worker_kind_ == WorkerKind::WriteDevice) {
                if (ok) {
                    self->show_toast("写入完成，设备已重启。", true);
                    self->set_status_text("状态: 写入成功", true, false);
                    self->write_device_button_.set_sensitive(false);
                } else {
                    self->show_toast("写入设备失败，请检查 adb 连接/权限。", false);
                    self->set_status_text("状态: 写入失败", false, false);
                }
            }

            if (self->worker_watch_id_ != 0) {
                g_source_remove(self->worker_watch_id_);
                self->worker_watch_id_ = 0;
            }
            g_spawn_close_pid(pid);
            self->worker_pid_ = 0;
            self->worker_kind_ = WorkerKind::Patch;
            self->update_patch_button_state();
        },
        this
    );
}

void MainWindow::schedule_toast_hide() {
    Glib::signal_timeout().connect_seconds_once(
        [this]() {
            toast_revealer_.set_reveal_child(false);
        },
        3
    );
}

void MainWindow::update_patch_button_state() {
    const bool ready = !source_path_.empty() &&
                       sn_entry_.get_text_length() == kSnLength &&
                       worker_pid_ == 0;
    patch_button_.set_sensitive(ready);
    if (!ready && worker_pid_ == 0) {
        write_device_button_.set_sensitive(false);
    }
}

std::string MainWindow::sanitize_sn(const std::string& text) const {
    std::string filtered;
    filtered.reserve(kSnLength);
    for (char ch : text) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            filtered.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            if (filtered.size() == kSnLength) {
                break;
            }
        }
    }
    return filtered;
}

void MainWindow::show_toast(const Glib::ustring& text, bool success) {
    const auto prefix = success ? "完成: " : "错误: ";
    toast_label_.set_text(prefix + text);
    toast_revealer_.set_reveal_child(true);
    schedule_toast_hide();
}

void MainWindow::set_status_text(const Glib::ustring& text, bool success, bool processing) {
    status_label_.set_text(text);
    status_is_success_ = success;
    is_processing_ = processing;
    status_light_.queue_draw();
}

bool MainWindow::on_status_light_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    const auto allocation = status_light_.get_allocation();
    const double width = static_cast<double>(allocation.get_width());
    const double height = static_cast<double>(allocation.get_height());
    const double radius = std::min(width, height) / 2.0 - 1.0;
    const double cx = width / 2.0;
    const double cy = height / 2.0;

    double r = 0.5;
    double g = 0.5;
    double b = 0.5;
    if (is_processing_) {
        r = 0.16;
        g = 0.62;
        b = 1.0;
    } else if (status_is_success_) {
        r = 0.25;
        g = 0.85;
        b = 0.39;
    } else {
        r = 1.0;
        g = 0.33;
        b = 0.33;
    }

    const double alpha = 0.35 + 0.65 * ((std::sin(pulse_phase_) + 1.0) / 2.0);
    cr->set_source_rgba(r, g, b, alpha);
    cr->arc(cx, cy, radius, 0.0, 2.0 * kPi);
    cr->fill();
    return true;
}

bool MainWindow::on_status_pulse_tick() {
    pulse_phase_ += 0.14;
    if (pulse_phase_ > 2.0 * kPi) {
        pulse_phase_ -= 2.0 * kPi;
    }
    status_light_.queue_draw();
    return true;
}

bool MainWindow::run_command_capture(const std::string& command, std::string* output) const {
    std::vector<std::string> args{"bash", "-lc", command};
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);

    gchar* std_out = nullptr;
    gchar* std_err = nullptr;
    gint exit_status = 0;
    GError* error = nullptr;
    const bool ok = g_spawn_sync(
        nullptr,
        argv.data(),
        nullptr,
        G_SPAWN_SEARCH_PATH,
        nullptr,
        nullptr,
        &std_out,
        &std_err,
        &exit_status,
        &error
    );

    std::string out_text;
    if (std_out != nullptr) {
        out_text += std_out;
        g_free(std_out);
    }
    if (std_err != nullptr && *std_err != '\0') {
        if (!out_text.empty() && out_text.back() != '\n') {
            out_text.push_back('\n');
        }
        out_text += std_err;
        g_free(std_err);
    }
    if (output != nullptr) {
        *output = out_text;
    }

    if (!ok) {
        if (error != nullptr) {
            if (output != nullptr) {
                *output = error->message;
            }
            g_error_free(error);
        }
        return false;
    }
    if (error != nullptr) {
        g_error_free(error);
    }
    return WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == 0;
}

void MainWindow::on_write_device_clicked() {
    if (worker_pid_ != 0) {
        return;
    }

    patch_button_.set_sensitive(false);
    write_device_button_.set_sensitive(false);
    show_toast("正在写入到设备...", true);
    set_status_text("状态: 写入设备中", true, true);
    worker_kind_ = WorkerKind::WriteDevice;

    std::vector<std::string> args{
        executable_path_,
        "--worker",
        "write_device",
        dest_path_
    };

    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);

    GError* error = nullptr;
    if (!g_spawn_async(
            nullptr,
            argv.data(),
            nullptr,
            static_cast<GSpawnFlags>(G_SPAWN_DO_NOT_REAP_CHILD),
            nullptr,
            nullptr,
            &worker_pid_,
            &error)) {
        const std::string message = error ? error->message : "无法启动后台子进程";
        if (error) {
            g_error_free(error);
        }
        worker_pid_ = 0;
        worker_kind_ = WorkerKind::Patch;
        show_toast("启动写入子进程失败: " + message, false);
        set_status_text("状态: 写入失败", false, false);
        update_patch_button_state();
        return;
    }

    worker_watch_id_ = g_child_watch_add(
        worker_pid_,
        [](GPid pid, gint status, gpointer data) {
            auto* self = static_cast<MainWindow*>(data);
            const bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
            if (self->worker_kind_ == WorkerKind::WriteDevice) {
                if (ok) {
                    self->show_toast("写入完成，设备已重启。", true);
                    self->set_status_text("状态: 写入成功", true, false);
                    self->write_device_button_.set_sensitive(false);
                } else {
                    self->show_toast("写入设备失败，请检查 adb 连接/权限。", false);
                    self->set_status_text("状态: 写入失败", false, false);
                }
            }
            if (self->worker_watch_id_ != 0) {
                g_source_remove(self->worker_watch_id_);
                self->worker_watch_id_ = 0;
            }
            g_spawn_close_pid(pid);
            self->worker_pid_ = 0;
            self->worker_kind_ = WorkerKind::Patch;
            self->update_patch_button_state();
        },
        this
    );
}

bool MainWindow::on_delete_event(GdkEventAny* any_event) {
    Gtk::MessageDialog dialog(
        *this,
        "是否清理 /tmp/snedit 下生成的内容？",
        false,
        Gtk::MESSAGE_QUESTION,
        Gtk::BUTTONS_NONE,
        true
    );
    dialog.add_button("取消", Gtk::RESPONSE_CANCEL);
    dialog.add_button("不清理", Gtk::RESPONSE_NO);
    dialog.add_button("清理并退出", Gtk::RESPONSE_YES);
    const int resp = dialog.run();

    if (resp == Gtk::RESPONSE_CANCEL) {
        return true;
    }
    if (resp == Gtk::RESPONSE_YES) {
        std::error_code ec;
        std::filesystem::remove_all("/tmp/snedit", ec);
    }
    return Gtk::Window::on_delete_event(any_event);
}
