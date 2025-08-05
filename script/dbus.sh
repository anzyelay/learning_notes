#!/bin/bash
myname=$(basename "$0")
module=${myname%.sh}
module=${module#fii-}

#readonly MESSAGE_BUS="--system"
readonly MESSAGE_BUS="--session"
readonly DEST="org.fii.${module}"
readonly INTERFACE="org.fii.${module}"
readonly OBJECT_PATH="/org/fii/${module}"
#readonly XMLFILE=/usr/share/xml/gdbus-tbox/org.fii.tbox.xml
readonly XMLFILE="/home/anzye/Desktop/tboxservice/gdbus/org.fii.tbox.xml"

function help() {
    cat <<EOF
Use ways:
        $myname method_name [args list]
        Eg: ./$myname method_name 47 'hello world' 65.32 true "123456"

            the base type support to input directly: int32, double, boolean, string
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
        echo "string:${param}"
    fi
}

function all_fun() {
    str="/<interface.*name="\"${INTERFACE}"\">/,/<\\/interface>/p"
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
    str="/<interface.*name="\"${INTERFACE}"\">/,/<\\/interface>/p"
    substr="/<method.*name="\"${fun}"\"/,/<\\/method>/p"
    ret=$(sed -En ${str} ${XMLFILE} | sed -En ${substr})
    if [ "$ret" != "" ]; then
        echo "${ret}" | sed "s/annotation name=\"org.freedesktop.DBus.Arg.Doc\"//" | sed '/doc:.*>/d'
    else
        echo "No this function, please check it with: $myname all_fun"
    fi
}

function monitor_prop() {
    dbus-monitor $MESSAGE_BUS "path=${OBJECT_PATH},member=PropertiesChanged"
}

function monitor_sig() {
    if [ $# -eq 0 ]; then
        dbus-monitor $MESSAGE_BUS "type='signal',interface='${INTERFACE}'",path=${OBJECT_PATH}
    else
        dbus-monitor $MESSAGE_BUS "type='signal',interface='${INTERFACE}',path=${OBJECT_PATH},member='$1'"
    fi
}

# 从XML中获取方法参数类型
function get_method_args_type() {
    local method_name="$1"
    str="/<interface.*name="\"${INTERFACE}"\">/,/<\\/interface>/p"
    substr="/<method.*name=\"${method_name}\"/,/<\\/method>/p"
    local args=$(sed -En "$str" "$XMLFILE" | sed -En "$substr" | grep "<arg.*direction=\"in\"" | sed -n 's/.*type="\([^"]*\)".*/\1/p')
    echo "$args"
}

function get_prop_args_type() {
    local prop_name="$1"
    str="/<interface.*name="\"${INTERFACE}"\">/,/<\\/interface>/p"
    substr="/<property.*name=\"${prop_name}\".*>/p"
    local args=$(sed -En "$str" "$XMLFILE" | sed -En "$substr" | grep "access=\".*write\"" | sed -n 's/.*type="\([^"]*\)".*/\1/p')
    echo "$args"
}

# 根据类型转换参数
function convert_arg_by_type() {
    local arg="$1"
    local type="$2"

    # 如果参数已经是DBus类型，则直接返回
    is_dbus_type "$arg" && echo "$arg" && return 0

    case "$type" in
        s)  # string
            echo "string:$arg"
            ;;
        i | u)  # integer
            if is_integer "$arg"; then
                echo "int32:$arg"
            else
                echo "错误：参数 '$arg' 不是有效的整数" >&2
                return 1
            fi
            ;;
        d)  # double
            if is_decimal "$arg"; then
                echo "double:$arg"
            else
                echo "错误：参数 '$arg' 不是有效的浮点数" >&2
                return 1
            fi
            ;;
        b)  # boolean
            if is_boolean "$arg"; then
                echo "boolean:$arg"
            else
                echo "错误：参数 '$arg' 不是有效的布尔值" >&2
                return 1
            fi
            ;;
        a\(*\))  # array with any type
            local base_type=${type#a(}
            base_type=${base_type%)}
            local items=()
            IFS=',' read -r -a items <<< "$arg"
            local dbus_items=""
            local converted=""
            local i=0;
            for item in "${items[@]}"; do
                if ! converted=$(convert_arg_by_type "$item" "$base_type"); then
                    echo "错误：参数 '$item' 不是有效的类型 $base_type" >&2
                    return 1
                fi
                if [ $i -eq 0 ]; then
                    dbus_items="${dbus_items}${converted}"
                else
                    dbus_items="${dbus_items},${converted#*:}"
                fi
                ((i++))
            done
            echo "array:${dbus_items}"
            ;;
        v*)
            local base_type=${type#v}
            local converted=""
            if ! converted=$(convert_arg_by_type "$arg" "$base_type"); then
                echo "错误：参数 '$arg' 不是有效的类型 $base_type" >&2
                return 1
            fi
            echo "variant:${converted}"
            ;;
        *)
            echo "未知类型: $type" >&2
            return 1
            ;;
    esac
}

function call_method() {
    local method="$1"
    shift

    # 获取方法参数类型
    local arg_types=()
    if [ "$method" == "org.freedesktop.DBus.Properties.Get" ]; then
        arg_types=("s" "s")
    elif [ "$method" == "org.freedesktop.DBus.Properties.Set" ]; then
        arg_types=("s" "s")
        vtype=$(get_prop_args_type "${2}")
        if [ "_$vtype" != "_" ]; then
            arg_types+=("v${vtype}")
        else
            echo "错误：属性 ${2##string:} 不可写" >&2
            return 1
        fi
    else
        arg_types=($(get_method_args_type "${method##*.}"))
    fi

    if [ ${#arg_types[@]} -ne $# ]; then
        echo "错误：方法 ${method##*.} 需要 ${#arg_types[@]} 个参数，但提供了 $# 个" >&2
        return 1
    fi

    local args=()
    local i=0
    local converted=""
    for arg in "$@"; do
        if ! converted=$(convert_arg_by_type "$arg" "${arg_types[$i]}"); then
            echo "参数转换失败: $converted" >&2
            return 1
        fi
        args+=("$converted")
        ((i++))
    done
    # echo "$method" "${args[@]}"
    ret=$(dbus-send $MESSAGE_BUS --print-reply=literal --reply-timeout=180000 --dest="${DEST}" "${OBJECT_PATH}" "$method" "${args[@]}")
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
        call_method ${METHOD} ${INTERFACE} "$1"
        ;;
    "set")
        METHOD="org.freedesktop.DBus.Properties.Set"
        call_method ${METHOD} ${INTERFACE} "$1" "$2"
        ;;
    "help" | "-h" | "-H" | "--help")
        help
        ;;
    *)
        MEMBER="${method}"
        METHOD="${INTERFACE}.${MEMBER}"
        call_method ${METHOD} "$@"
        ;;
esac

exit 0
