## appdata.xml
参考AppStream
- translation : 表示该应用使用到的翻译文件，如下为data/po/networking-plug.pot, 生成的翻译文本为 *.mo 文件

```xml
<?xml version="1.0" encoding="UTF-8"?>
<component type="addon">
  <id>com.patapua.settingboard.network</id>
  <extends>com.patapua.settingboard</extends>
  <name>Network Settings</name>
  <summary>Manage network devices and connectivity</summary>
  <icon type="stock">preferences-system-network</icon>
  <translation type="gettext">networking-plug</translation>
  <releases>
    <release version="1.0.0" date="2021-09-18" urgency="medium">
      <description>
        <p>Minor updates</p>
        <ul>
          <li>init </li>
        </ul>
      </description>
    </release>
    <release > ... </release>
  </releases>
  <screenshots>
    <screenshot type="default">
      <image>https://raw.githubusercontent.com/elementary/switchboard-plug-network/master/data/screenshot.png</image>
    </screenshot>
  </screenshots>
  <url type="homepage">https://elementary.io</url>
  <url type="bugtracker">https://github.com/elementary/switchboard-plug-network/issues</url>
  <metadata_license>CC0-1.0</metadata_license>
  <project_license>GPL-3.0+</project_license>
</component>
```

---
## gresource.xml

```xml
<gresources>
  <gresource prefix="/prefix/path/to/file">
    <file alias="left.svg" compressed="true" preprocess="xml-stripblanks">left.svg</file>
  </gresource>
</gresources>
```
