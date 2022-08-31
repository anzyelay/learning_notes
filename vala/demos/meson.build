project(
    'com.anzye.example',
    'vala','c',
    version: '1.0.0'
)

glib_dep = dependency('glib-2.0', version: '>=2.32')
gio_dep = dependency('gio-2.0')
gio_unix_dep = dependency('gio-unix-2.0')
gmodule_dep = dependency('gmodule-2.0')
gtk_dep = dependency('gtk+-3.0', version: '>=3.10')
gee_dep = dependency('gee-0.8')
granite_dep = dependency('granite', version: '>=5.4.0')
handy_dep = dependency('libhandy-1', version: '>=0.83.0')
m_dep = meson.get_compiler('c').find_library('m', required : false)
dbus_dep = dependency ('dbus-glib-1')



subdir('demo_args')
subdir('demo_dbus')
subdir('demo_drag_and_drop')
