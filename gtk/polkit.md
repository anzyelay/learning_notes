## 写policy文件
1. policy文件说明
- action id, 程序中使用id指明使用哪个action
- key:
  - org.freedesktop.policykit.exec.path: 添加具体可执行程序
  - org.freedesktop.policykit.imply: 添加另一个action
    ```xml
    <policyconfig>
    <vendor>elementary</vendor>
    <vendor_url>https://elementary.io/</vendor_url>

    <action id="com.patapua.appstore.update">
        <description gettext-domain="@GETTEXT_PACKAGE@">Update software</description>
        <message>Authentication is required to update software</message>
        <icon_name>package-x-generic</icon_name>
        <defaults>
        <allow_any>auth_admin</allow_any>
        <allow_inactive>auth_admin</allow_inactive>
        <allow_active>yes</allow_active>
        </defaults>
        <annotate key="org.freedesktop.policykit.exec.path">/usr/bin/localectl</annotate>
        <annotate key="org.freedesktop.policykit.imply">org.freedesktop.packagekit.system-update</annotate>
        <annotate key="org.freedesktop.policykit.imply">com.patapua.appstore.add_key</annotate>
    </action>

    <action id="com.patapua.appstore.add_key">
        <description>Add public key</description>
        <message>Authentication is required to add public key to system</message>
        <icon_name>package-x-generic</icon_name>
        <defaults>
        <allow_any>no</allow_any>
        <allow_inactive>no</allow_inactive>
        <allow_active>auth_admin_keep</allow_active>
        </defaults>
        <annotate key="org.freedesktop.policykit.exec.path">/usr/bin/gpg</annotate>
    </action>

    </policyconfig>

    ```
1. 安装policy, /usr/share/polkit-1/actions/
    ```meson
    # Install policy
    i18n.merge_file(
        input: 'my.policy.in',
        output: meson.project_name() + '.policy',
        po_dir: join_paths(meson.source_root (), 'po', 'extra'),
        install_dir: join_paths(get_option('datadir'), 'polkit-1', 'actions'),
        install: true
    )
    ```

2. vala代码中如何使用？
- 添加依赖： `dependency ('polkit-gobject-1')`
- 使用**action id**创建permission对象
   ```vala
        Polkit.Permission ?permission = null;
        try {
            permission = new Polkit.Permission.sync (
                "com.patapua.appstore.add_key",
                new Polkit.UnixProcess (Posix.getpid ())
            );
        } catch (Error e) {
            critical ("Can't get permission to add_key without prompting for admin: %s", e.message);
            return false;
        }
   ```
- 弹窗验证：
   ```vala
        if (!permission.allowed) {
            try {
                permission.acquire (null);
            } catch (Error e) {
                critical (e.message);
                return false;
            }
        }
    ```
- 验证通过使用pkexec运行命令
    ```vala
        if (permission.allowed) {
            //warning ("execute /usr/bin/gpg --yes --batch --no-tty --dearmor -o %s %s".printf (gpg_path, local_file));
            string output;
            int status;
            var cli = "/usr/bin/gpg";
            try {
                Process.spawn_sync (null,
                    {"pkexec", cli, "--yes", "--batch", "--no-tty","--dearmor", "-o", gpg_path, local_file ()},
                    Environ.get (), SpawnFlags.SEARCH_PATH, null, out output, null, out status);

                if (output != "") {
                    critical ("gpg failed to add key");
                }
                return true;
            } catch (Error e) {
                critical ("gpg failed to excute %s", e.message);
            }
        }
   ```
