# 指纹解锁

gxde-lock 可以用指纹代替密码解锁。指纹与密码同时可用，任何时候都能改输密码。

| 锁屏 | 支持情况 |
| --- | --- |
| 现代锁屏 `gxde-lock-neo` | 支持，有提示 |
| 经典锁屏 `gxde-lock-classic` | 可以解锁，但界面没有指纹提示 |

指纹验证由 fprintd 和 `pam_fprintd` 完成，gxde-lock 自己不读取、不保存指纹数据。

## 1. 准备

需要一个 [libfprint 支持的指纹读取器](https://fprint.freedesktop.org/supported-devices.html)，
并安装 fprintd 和它的 PAM 模块：

```sh
# Arch Linux（fprintd 自带 pam_fprintd）
sudo pacman -S fprintd

# Debian / GXDE OS
sudo apt install fprintd libpam-fprintd
```

fprintd 由 D-Bus 按需启动，不需要 `systemctl enable`。确认读取器被识别：

```sh
fprintd-list "$USER"
```

输出里出现设备名（例如 `found 1 devices`）即可；`No devices available`
说明读取器不受支持或驱动没有加载。

## 2. 录入指纹

在已登录的桌面会话里，以**自己的身份**运行（不要加 `sudo`）：

```sh
fprintd-enroll                       # 由 fprintd 自动挑一根尚未录入的手指
fprintd-enroll -f left-index-finger  # 指定手指
```

按提示反复按压或划过读取器，直到出现 `Enroll result: enroll-completed`。
所需次数取决于设备，通常 5～15 次。录入自己的指纹时 polkit 会先要求输入
一次自己的密码。

可选的手指名称：`left-thumb`、`left-index-finger`、`left-middle-finger`、
`left-ring-finger`、`left-little-finger`，以及对应的 `right-*`。建议至少录入
两根手指，受伤或手指潮湿时还有备用。

录入后先在命令行确认能认出来：

```sh
fprintd-verify      # 按下已录入的手指，应显示 verify-match
fprintd-list "$USER"
```

管理员给其他用户录入需要管理员权限：

```sh
sudo fprintd-enroll -f right-index-finger alice
```

删除指纹：

```sh
fprintd-delete "$USER"                         # 删除自己全部指纹
fprintd-delete "$USER" -f right-index-finger   # 只删一根（fprintd ≥ 1.92）
```

## 3. 开启

gxde-lock **没有单独的开关**。下面三个条件同时满足时自动启用：

1. 已安装 PAM 服务文件 `/etc/pam.d/gxde-lock-fingerprint`；
2. fprintd 找到了读取器；
3. 当前用户录入过指纹。

gxde-lock 的软件包（Arch 和 Debian 打包都使用 `INSTALL_PAM_CONFIGURATION=ON`）
会安装 `/etc/pam.d/gxde-lock-fingerprint`。检查一下：

```sh
ls -l /etc/pam.d/gxde-lock-fingerprint
```

如果是从源码构建并手动安装、没有这个文件，从源码树复制：

```sh
# Arch 及其他发行版
sudo install -Dm644 services/gxde-lock-fingerprint.pam /etc/pam.d/gxde-lock-fingerprint
# Debian / GXDE OS
sudo install -Dm644 services/debian.gxde-lock-fingerprint.pam /etc/pam.d/gxde-lock-fingerprint
```

**不需要重启锁屏服务。** 锁屏每次显示时都会重新检查这三个条件，录入完成后
下次锁屏就能用。

FreeBSD 不安装这个文件，因为 fprintd 不在 FreeBSD 基本系统里。

## 4. 使用

- 锁屏一出现就开始等待指纹，包括还停在时钟画面的时候，不必先唤出密码框。
- 锁屏隐藏时（空闲待命的锁屏进程）不占用读取器。

**识别失败时**

| 情况 | 表现 |
| --- | --- |
| 手指不匹配 | 和输错密码一样抖动、边缘红光，显示 `pam_fprintd` 的提示（如“指纹匹配失败”），继续等待 |
| 连续失败达到次数（默认 3 次） | 显示“指纹多次不匹配，请输入密码”，停止等待 |
| 一段时间没有按手指（默认 30 秒） | 静默重新开始等待，界面不变 |

停止等待后，进入或离开密码输入界面会重新启用指纹。

## 5. 调整

次数和超时在 `/etc/pam.d/gxde-lock-fingerprint` 的 `pam_fprintd.so` 行上设置，
参见 `man pam_fprintd`：

```
auth        required    pam_fprintd.so  max-tries=5 timeout=60
```

`timeout` 请不要小于 10 秒。gxde-lock 用“失败来得快不快”区分“次数用完”和
“超时”：太短的超时会被当成次数用完，甚至被判定为指纹不可用。

请保持 `pam_fprintd.so` 前面**没有** `-` 前缀。`-` 表示模块缺失时跳过，
那样这个栈在没有指纹模块时可能直接放行。

## 6. 关闭

- **单个用户**：`fprintd-delete "$USER"` 删除自己的指纹。
- **整台机器**：删除 `/etc/pam.d/gxde-lock-fingerprint`。Debian 会记住被删除的
  配置文件，升级时不会装回；Arch 升级 gxde-lock 时会重新安装它，需要再次删除，
  或者在 `/etc/pacman.conf` 中加入 `NoExtract = etc/pam.d/gxde-lock-fingerprint`。

## 7. 注意事项

- **钥匙环不会自动解锁。** GNOME Keyring、KWallet 等靠密码解锁，用指纹解锁时
  PAM 拿不到密码，钥匙环仍保持锁定，之后会弹窗要求输入钥匙环密码。
- **账户锁定同样生效。** 在 Arch 的 PAM 栈里，`pam_faillock` 因为密码多次
  输错锁定账户期间，指纹也会被拒绝。
- 锁屏的 PAM 栈不检查 `/etc/nologin`，也不拒绝 root：它们只应阻止新的登录，
  不能把已经登录的用户锁在会话外面。
- 指纹不能用来修改密码（`password required pam_deny.so`）。
- 指纹是密码之外的另一种凭据，并不是双因素认证：两者任一通过即可解锁。

## 8. 排错

先确认 fprintd 本身正常：

```sh
fprintd-list "$USER"
fprintd-verify
```

再单独测试 gxde-lock 的 PAM 服务（需要 `pamtester`：Debian 官方仓库有，Arch 在 AUR）：

```sh
pamtester gxde-lock-fingerprint "$USER" authenticate
```

查看日志：

```sh
journalctl -b --user | grep -iE 'fingerprint|fprint'     # 锁屏（X11 会话也可能在 ~/.xsession-errors）
journalctl -b -u fprintd                                 # 读取器
```

常见现象：

| 现象 | 可能原因 |
| --- | --- |
| 输入框只显示“密码”，没有“密码或指纹” | 三个启用条件之一不满足；用上面的命令逐一检查 |
| 日志出现 `gxde-lock-fingerprint asked for input` | `/etc/pam.d/gxde-lock-fingerprint` 被改成了会提示输入的栈（例如 include 了密码栈）；gxde-lock 不会回答提示，会放弃这次指纹验证 |
| 每次按手指都提示不匹配 | 先用 `fprintd-verify` 确认；必要时删除后重新录入 |
| 读取器被其他程序占用 | 同一时间只有一个程序能使用读取器；关掉其他正在等待指纹的程序（例如 `sudo` 的指纹提示） |
