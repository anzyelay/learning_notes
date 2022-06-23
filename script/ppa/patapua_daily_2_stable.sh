#!/bin/bash
channels=$@
codenames=(focal)
archs=(amd64 i386)

ppa_root="/home/ppa"
patapua_root="${ppa_root}/patapua"

channel="daily"
update_channel="stable"
pathdir="${patapua_root}/${channel}/ubuntu/"

echo "-----------------------WARNING---------------------------------"
echo "---------------------------------------------------------------"
echo "---------------------------------------------------------------"
read -r -p "!!!! Are you sure to update ${patapua_root}/${channel} packages to its ${update_channel} ? !!!!!!!! [Yes/no] " input
case $input in
	Yes)
		;;
	[Nn][Oo]|[Nn])
		echo "No, quit"
		exit 0
		;;
	*)
		echo "Invalid input, input as tips or quit update!"
		exit 1
		;;
esac

for Packages in `find ${pathdir}/dists/focal/main/binary-amd64 -name Packages`; do
	echo "use file:$Packages to parse packages"
	sed -n '/Filename: /p' ${Packages}  | awk '{ print $2 }'  > /tmp/pkg.tmp
	declare -i pkg_cnt=0
	for file in `cat /tmp/pkg.tmp`;do 
		let pkg_cnt++
		src_file=${pathdir}/${file}
		dst_file=${src_file/$channel/${update_channel}}
		# echo $pkg_cnt  "$src_file --> $dst_file"
		install  -CDv $src_file  $dst_file
		if [ $? -ne 0 ];then
			rm /tmp/pkg.tmp
			exit 1
		fi
	done
	echo "${pkg_cnt} packages have been installed"
done
rm /tmp/pkg.tmp
