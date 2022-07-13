#!/bin/bash  
#set -x
force=0
find_git_repository(){
	if [ ! -d $1 ];then
		return 1
	fi
	if [ -d $1/.git ];then
		GIT_DIRS+=($1)
		return 0
	else
		for dir in `ls $1`;do
			find_git_repository $1/$dir
		done
	fi
}

update_pkg(){
	# make sure a git repository here
	if [ ! -d $1/.git ];then
		echo "not a valid git repository"
		return 0
	fi
	cd $1

	BRANCH=${2:-develop}
	echo $BRANCH

	# checkout to ${BRANCH} branch,and check it need to update
	git branch  | grep "${BRANCH}" > /dev/null 
	if [ $? -eq 0 ];then
		git branch  | grep "* ${BRANCH}" > /dev/null || git checkout ${BRANCH}
		git fetch
		git status | grep "Your branch is up to date with 'origin/${BRANCH}'" > /dev/null
		if [ $? -eq 0 -a $force -eq 0 ];then
			echo "needless to update $1"
			cd - > /dev/null; return 0
		fi
		git pull origin
	else
		git checkout ${BRANCH}
	fi

	# start to build the deb package here
	echo "jideos123" | sudo -S apt build-dep .
	dpkg-buildpackage -rfakeroot -tc --no-sign  
	if [ $? -ne 0 ];then
		cd - >/dev/null; return 1
	fi

	cd ..
	# archive the deb and transfer to ppa
	TARNAME=arm_`basename $1`.tar
	find .  -maxdepth 1 -type f | xargs tar cvf ${TARNAME} --exclude=*.tar
	tar tf $TARNAME | xargs rm && scp $TARNAME ppa@192.168.16.174:/tmp/ && rm ${TARNAME}
}

while getopts f var
do
  case $var in
  f)  force=1;;
  \?)     echo '$0 [-f] path'; exit 1;;
  esac
done
shift `expr $OPTIND - 1`

if [ $# -gt 0 ];then
	GIT_DIRS=(${1%/})
else
	find_git_repository `pwd`
fi

for dir in ${GIT_DIRS[@]}
do
	echo start to updating $dir at branch $2
	update_pkg $dir $2
done

