![HEADER](./docs/img/header.png)

![CONTRIBUTORS](https://img.shields.io/github/contributors/GXDE-OS/gxde-lock.svg?style=plastic)
![ISSUES](https://img.shields.io/github/issues/GXDE-OS/gxde-lock.svg?style=plastic)
![LICENSE](https://img.shields.io/badge/license-GPLv3-blue)
![](https://img.shields.io/badge/made_with-love-red)


<br />
<div align="center">
  <h3 align="center">GXDE Lock</h3>

  <p align="center">
    GXDE OS 的会话内锁屏，支持 X11 与 Wayland。
    <br />
    <a href="https://gitee.com/GXDE-OS/gxde-lock/issues">提交Issue</a>
    &middot;
    <a href="https://gitee.com/GXDE-OS/gxde-lock/issues">请求新功能</a>
  </p>
</div>

[English](./README.md)

## 关于本项目
`gxde-lock` 是从 [GXDE Display Manager](https://github.com/GXDE-OS/gxde-display-manager)
（`GXDM`，SDDM 的分支）中拆分出来的锁屏。显示管理器部分（守护进程、登录界面、
会话启动辅助程序、主题）已全部移除，只保留锁定与解锁正在运行的会话所需的部分：

| 组件 | 安装位置 | 源码 | 用途 |
| --- | --- | --- | --- |
| 经典锁屏 | `/usr/bin/gxde-lock-classic` | `src/lock-classic` | DTK 控件锁屏；也是固定的自启动与 D-Bus 入口，选择现代样式时交给现代锁屏。 |
| 现代锁屏 | `/usr/bin/gxde-lock-neo` | `src/lock-neo` | 基于 `src/lock-common` 的 Qt Quick 锁屏。 |
| 认证辅助程序 | `<libexec>/gxde-lock-auth-helper` | `src/auth-helper` | 以被锁定用户的身份运行 `gxde-lock` PAM 栈，从不打开会话。 |
| root 辅助程序 | `<libexec>/gxde-lock-system` | `src/system` | 按需激活的系统总线服务，保存所有用户共享的设置并执行电源操作。 |

两种锁屏都支持 X11（全屏窗口并抓取键盘）和 Wayland（合成器支持时使用
`ext-session-lock-v1`，否则使用 LayerShellQt 的 `zwlr_layer_shell_v1`），
都可以用密码解锁；`fprintd` 有指纹设备且已录入指纹时也可以用指纹解锁。

### 工具链
#### 构建时
* CMake 3.21+（Debian 主机上也可用 Bazel/Bzlmod）以及支持 C++20 的编译器
* PAM、X11、Xcursor、Xfixes、XInput、XRandR、XTest、Xext
* **Qt 6.5+**：Core、DBus、Gui、Widgets、Network、Qml、Quick、Svg、Xml、WaylandClient
* **DTK**：DTK2Widget-Qt6、DTK6 Core、DTK6 DLog
* gsettings-qt6、LayerShellQt、wayland-protocols、wayland-scanner

***注意**：完整的 Debian 依赖见 [`debian/control`](./debian/control)。*

#### 运行时
* PAM、logind（或 elogind）、D-Bus
* AccountsService
* X11 会话，或支持 `ext-session-lock-v1` 或 `zwlr_layer_shell_v1` 的 Wayland 合成器
* `gxde-lock-neo` 需要 Qt Quick、Qt Quick Controls/Layouts/Effects 和 Qt Virtual Keyboard 的 QML 模块

## 开始上手
### 从源码构建（CMake）
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j4
DESTDIR="$PWD/build/stage" cmake --install build   # 先检查
sudo cmake --install build
```

| 选项 | 默认值 | 作用 |
| --- | --- | --- |
| `BUILD_NEO_LOCK` | `ON` | 构建 `gxde-lock-neo`。 |
| `LOCK_BUILD_TESTS` | `OFF` | 构建锁屏后端测试（`LockStyle`、`LockBackend`、`LockMedia`、`LockServices`、`LockFingerprint`）。 |
| `INSTALL_PAM_CONFIGURATION` | `ON` | 安装 `/etc/pam.d/gxde-lock` 与 `gxde-lock-fingerprint`。 |
| `ENABLE_JOURNALD` | `ON` | 认证辅助程序的日志写入 journald。 |
| `NO_SYSTEMD` / `USE_ELOGIND` | `OFF` | 不使用 systemd 构建，或使用 elogind。 |

### 从源码构建（Bazel）
```bash
bazel build --spawn_strategy=local //:gxde-lock-dist
```
Bazel 构建使用 Debian 路径下的主机 Qt 与 DTK（见 `MODULE.bazel`）。

### 打包
```bash
./build-deb -d          # Debian / GXDE OS
cd arch && makepkg -si  # Arch Linux
```

## 使用方式
软件包会安装 XDG 自启动项，锁屏服务随会话启动
（`gxde-lock-classic --daemon --auto-enroll`），并且：

* 注册 `org.freedesktop.ScreenSaver` 与 Deepin 的 `com.deepin.dde.lockFront`，
  桌面的“锁定屏幕”操作会调用它；
* 通过 KGlobalAccel 注册 Super+L（已被其他程序占用时不会抢占）。

立即锁屏：
```bash
gxde-lock-classic                # 或：gxde-lock-neo
```

不锁定会话的预览（任意密码均可解锁，`Esc` 退出）：
```bash
gxde-lock-neo --test-mode
gxde-lock-neo --test-mode --switch
```

### 锁屏样式
由 `/etc/gxde-lock.conf` 中的 `[General] LockScreenStyle` 选择：

```ini
[General]
LockScreenStyle=modern
```

`modern`（默认）运行 `gxde-lock-neo`，`classic` 运行 `gxde-lock-classic`。
没有该键时使用 `/etc/gxde-lock/OMG.conf` 中的 `[NextFeatReq] SwitchLkScrStyle`。
未安装 `gxde-lock-neo` 时总是使用经典样式。通过 `SetLockScreenStyle` 在运行时
切换会在下次解锁后生效：正在运行的锁屏释放其 D-Bus 名称后 exec 另一个锁屏，
会话不会因此处于未锁定状态。

### D-Bus接口
应用程序使用会话总线：

| 项 | 值 |
| --- | --- |
| 总线 | 会话总线 |
| 服务 | `top.gxde.Lock` |
| 对象路径 | `/top/gxde/Lock` |
| 接口 | `top.gxde.Lock` |

| 方法 | 说明 |
| --- | --- |
| `TryEnrollLkScr(bool enabled) -> bool` | 注册或撤销 Super+L；不会抢占已有的 Super+L 绑定。 |
| `UnregisterLkScr()` | 撤销 Super+L，并停止当前锁屏进程的自动重新注册。 |
| `LkScrStat() -> bool` | gxde-lock 是否持有 Super+L。 |
| `Show()` | 锁定屏幕。 |
| `SetCursor(string theme) -> bool` | 为所有用户设置锁屏光标主题，主题必须已安装。 |
| `SetWallpaperGXDEDefault() -> bool` | 使用 GXDE 默认壁纸作为全局锁屏壁纸。 |
| `SetWallpaperDDELockDefault() -> bool` | 使用内置的 DDE 锁屏背景作为全局锁屏壁纸。 |
| `SetWallpaper(string path) -> bool` | 从本地路径或 `file://` URL 设置自定义全局锁屏壁纸。 |
| `ClearWallpaper() -> bool` | 清除全局锁屏壁纸。 |
| `SetLockWallpaperOverride(string path) -> bool` | 设置当前用户自己的锁屏壁纸。 |
| `ClearLockWallpaperOverride() -> bool` | 清除当前用户的锁屏壁纸。 |
| `LockWallpaperOverride() -> string` | 当前用户的锁屏壁纸，没有时为空字符串。 |
| `SetLockScreenStyle(string style) -> bool` | 为所有用户保存 `classic` 或 `modern`；对应的锁屏未安装时失败。 |
| `LockScreenStyle() -> string` | 实际生效的锁屏样式。 |

信号：`LkScrStatChanged(bool)`、`LockScreenStyleChanged(string)`。

示例：
```bash
busctl --user call top.gxde.Lock /top/gxde/Lock top.gxde.Lock TryEnrollLkScr b true
busctl --user call top.gxde.Lock /top/gxde/Lock top.gxde.Lock Show
busctl --user call top.gxde.Lock /top/gxde/Lock top.gxde.Lock SetWallpaper s /path/to/image.jpg
busctl --user call top.gxde.Lock /top/gxde/Lock top.gxde.Lock SetLockWallpaperOverride s /path/to/image.jpg
busctl --user call top.gxde.Lock /top/gxde/Lock top.gxde.Lock SetLockScreenStyle s classic
```

所有用户共享的设置（样式、全局壁纸、光标主题）需要 root 权限，会话接口会把它们
转发给 `gxde-lock-system`。它在**系统**总线上持有 `top.gxde.Lock`，提供内部接口
`top.gxde.Lock.System`：写入 `/etc/gxde-lock.conf` 与
`/var/lib/gxde-lock/state.conf`，把自定义壁纸（以文件描述符传入，最大 128 MiB）
复制到 `/var/lib/gxde-lock`，空闲一分钟后退出。请勿直接调用。

用户锁屏壁纸的优先级依次为：该用户的 `LockWallpaperOverride`
（`~/.local/share/gxde-lock/lock-wallpaper-override`）、Deepin Accounts 的
`GreeterBackground`/`BackgroundFile`/`DesktopBackgrounds`、AccountsService、
GSettings 桌面壁纸，最后是全局锁屏壁纸或 GXDE 默认壁纸。

### 指纹解锁
见 [docs/FINGERPRINT.md](./docs/FINGERPRINT.md)。

## 贡献
欢迎提交 Issue 与 Pull Request。

### 打包注意事项
请将 `data/configs/OMG.conf` 安装到 `/etc/gxde-lock/`（CMake 会自动安装），
并将 `services/` 下的 PAM 配置安装为 `/etc/pam.d/gxde-lock` 与
`/etc/pam.d/gxde-lock-fingerprint`。

***注意**：gxde-lock 来自基于 SDDM 的 GXDM，SDDM 的贡献者见[这里](https://github.com/sddm/sddm/graphs/contributors?all=1)。*

## 许可证
本项目使用 [GNU GENERAL PUBLIC LICENSE Version 3](./LICENSE) 授权。
继承自 SDDM 的文件保留其 GPL-2.0-or-later 文件头。

`src/lock-classic` 包含来自 GXDE `session-ui`
（[GXDE-OS/gxde-session-ui](https://gitee.com/GXDE-OS/gxde-session-ui)）的代码，
除文件头另有说明外以 GPL-3.0 授权。

本项目包含来自 [CharOfString/samoyed-greeter](https://gitee.com/CharOfString/samoyed-greeter) 的代码，除文件头另有说明外以 GPL-3.0 授权。

锁屏字体为 [Maven Pro](https://github.com/googlefonts/mavenproFont)，以 [SIL Open Font License 1.1](https://github.com/googlefonts/mavenproFont/blob/main/OFL.txt) 授权。

## 鸣谢
* **SDDM**: https://github.com/sddm/sddm
* **GXDE Display Manager**: https://github.com/GXDE-OS/gxde-display-manager
* **GXDE Session UI**: https://gitee.com/GXDE-OS/gxde-session-ui
* **Samoyed Greeter**: https://gitee.com/CharOfString/samoyed-greeter
* **BEST-Readme-Template**: https://github.com/othneildrew/Best-README-Template
