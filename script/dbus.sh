#!/bin/bash
myname=$(basename $0)
module=${myname%.sh}
module=${module#fii-}

MESSAGE_BUS="--system"
DEST=org.fii.${module}
INTERFACE=/org/fii/${module}
XMLFILE=/usr/share/xml/gdbus-tbox/org.fii.tbox.xml
PREFIX_METHOD="org.fii.${module}"

function help() {
    cat <<EOF
Use ways:
        $myname method_name [args list]
        Eg: ./$myname method_name 47 'hello world' 65.32 true

            the base type support: int, string, double, bool, you can input directly
            the complex type should be like this:
                variant:int32:-8
                variant:boolean:false
                array:string:'"1st item"','"next item"',"last_item"
                dict:string:int32:"one",1,"two",2,"three",3

General methods:
        - all_fun:              罗列所有method接口功能
        - info method_name:     查看接口调用详情
        - mon | monitor:        监听所有属性变更事件
        - sig [signal_name]:    监听信号(无参数监听所有信号)
        - get prop_name:             获取属性值
        - set prop_name type:value:  设置属性值（如果可写）

Other special methods/signals/properities list as bellow:
EOF
    all_fun
}

function is_integer() {
    local re='^[-+]?[0-9]+$'
    if [[ $1 =~ $re ]]; then
        return 0
    else
        return 1
    fi
}

function is_decimal() {
    local re='^[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?$'
    if [[ $1 =~ $re ]]; then
        return 0
    else
        return 1
    fi
}

function is_boolean() {
    param="$1"
    local ret=0
    case "$param" in
    "true" | "TRUE" | "True")
        ;;
    "false" | "FALSE" | "False")
        ;;
    *)
        ret=1
        ;;
    esac
    return $ret
}

function is_dbus_type() {
    local param="$1"
    local prefixes=("string:" "dict:" "variant:" "boolean:" "double:" "array:" "int32:")
    local match=1
    for prefix in "${prefixes[@]}"; do
        if [[ "$param" == "$prefix"* ]]; then
            match=0
            break
        fi
    done
    return $match
}

# Converts a given parameter to its corresponding D-Bus type.
function to_dbus_type() {
    local param="$1"
    if is_dbus_type "$param"; then
        echo "$param"
    elif is_boolean "$param"; then
        echo "boolean:$param"
    elif is_integer "$param"; then
        echo "int32:$param"
    elif is_decimal "$param"; then
        echo "double:$param"
    else
        echo "string:'${param}'"
    fi
}

function all_fun() {
    str="/<interface.*name="\"${DEST}"\">/,/<\\/interface>/p"
    local mtds=$(sed -En ${str} ${XMLFILE} | sed -n "s/<method name=\"\(.*\)\">/- \1/p")
    if [ "_$mtds" != "_" ]; then
        echo "  ==Methods=="
        echo "$mtds"
        echo ""
    fi
    local sigs=$(sed -En ${str} ${XMLFILE} | sed -n "s/<signal name=\"\(.*\)\">/- \1/p")
    if [ "_$sigs" != "_" ]; then
        echo "  ==Signals=="
        echo "$sigs"
        echo ""
    fi
    local props=$(sed -En ${str} ${XMLFILE} | grep "<property name=.*>")
    if [ "_$props" != "_" ]; then
        echo "  ==Properties=="
        echo "$props"
        echo ""
    fi
}

function info_fun() {
    fun="$1"
    str="/<interface.*name="\"${DEST}"\">/,/<\\/interface>/p"
    substr="/<method.*name="\"${fun}"\"/,/<\\/method>/p"
    ret=$(sed -En ${str} ${XMLFILE} | sed -En ${substr})
    if [ "$ret" != "" ]; then
        echo "${ret}" | sed "s/annotation name=\"org.freedesktop.DBus.Arg.Doc\"//" | sed '/doc:.*>/d'
    else
        echo "No this function, please check it with: $myname all_fun"
    fi
}

function monitor_prop() {
    dbus-monitor $MESSAGE_BUS "path=${INTERFACE},member=PropertiesChanged"
}

function monitor_sig() {
    if [ $# -eq 0 ];then
        dbus-monitor $MESSAGE_BUS path=${INTERFACE}
    else
        dbus-monitor $MESSAGE_BUS path=${INTERFACE} member="$1"
    fi
}

function call_method() {
    local method="$1"
    shift
    local args=()
    for arg in "$@"; do
        dt="$(to_dbus_type "$arg")"
        args+=("$dt")
    done
    #echo "$method" "${args[@]}"
    ret=$(dbus-send $MESSAGE_BUS --print-reply=literal --dest=${DEST} ${INTERFACE} "$method" "${args[@]}")
    echo "$ret"
}

if [ $# -eq 0 ]; then
    help
    exit 1
fi

method=$1
shift

case "$method" in
    "all_fun")
        all_fun
        ;;
    "info")
        info_fun $1
        ;;
    "mon" | "monitor")
        monitor_prop
        ;;
    "sig")
        monitor_sig $@
        ;;
    "get")
        METHOD="org.freedesktop.DBus.Properties.Get"
        call_method ${METHOD} string:${DEST} string:$1
        ;;
    "set")
        METHOD="org.freedesktop.DBus.Properties.Set"
        call_method ${METHOD} string:${DEST} string:$1 variant:$2
        ;;
    "help" | "-h" | "-H" | "--help")
        help
        ;;
    *)
        METHOD="${PREFIX_METHOD}.${method}"
        call_method ${METHOD} "$@"
        ;;
esac

exit 0
