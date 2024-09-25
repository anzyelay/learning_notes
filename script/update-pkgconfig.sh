#!/bin/bash

# 更新pkgconfig文件的安装目录(支持递归)
update_pkgconfig_prefix()
{
    filePath=$1
    prefixDir=$2
    for file in `ls -a $filePath`
    do
        if [ -d ${filePath}/$file ]
        then
            # 子目录(递归)
            if [[ $file != '.' && $file != '..' ]]
            then
                update_pkgconfig_prefix ${filePath}/$file $prefixDir
            fi
        else
            filename=${filePath}/$file
            # 查找.pc后缀的文件
            if [ "${filename##*.}"x = "pc"x ]
            then
                # 查找设置prefix的所在行号
                prefix_line=`grep -n "prefix=/" $filename | cut -d ":" -f 1`
                # 新的设置内容
                prefix_str="prefix=$prefixDir"
                # 替换
                sed -i "$prefix_line c $prefix_str" $filename
            fi
        fi
    done
}

workdir=$(cd $(dirname $0); pwd)
update_pkgconfig_prefix $workdir/lib/pkgconfig $workdir
