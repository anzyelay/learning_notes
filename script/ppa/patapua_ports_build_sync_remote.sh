#!/bin/bash
if [ $# -eq 0 ];then
	echo "You can input a arg like: stable, os-patches or daily"
	exit 0
fi
# channels=(stable os-patches daily)
channels=$@
codenames=(focal)
archs=(arm64)

ppa_root="/home/ppa"
patapua_root="${ppa_root}/patapua-ports"
conf_file="${ppa_root}/conf/${channels[0]}/aptftp.conf"

for channel in ${channels[@]}
do
  channel_path=${patapua_root}/${channel}/ubuntu
  if [ -d ${channel_path} ]; then
    echo "entering ${channel_path}"
    cd ${channel_path}
  else
    echo "${channel_path} not exist" 1>&2
    exit 1
  fi
  
  for codename in ${codenames[@]}
  do
    codename_path=${channel_path}/dists/${codename}
    if [ -d ${codename_path} ]; then
      for arch in ${archs[@]}
      do
        arch_path=${codename_path}/main/binary-${arch}
        if [ -d ${arch_path} ]; then
          echo "generating Packages for ${arch_path}"
          if [ -e ${arch_path}/Packages ]; then
            rm -rf ${arch_path}/Packages
          fi
          # apt-ftparchive packages -a ${arch} pool > ${arch_path}/Packages
          # dpkg-scanpackages -a ${arch} pool > ${arch_path}/Packages
          dpkg-scanpackages pool > ${arch_path}/Packages
          
          # echo "generating Sources for ${arch_path}"
          # if [ -e ${arch_path}/Sources ]; then
          #   rm -rf ${arch_path}/Sources
          # fi
          # apt-ftparchive packages . > ${arch_path}/Sources
        else
          echo "${arch_path} not exist" 1>&2
          exit 1
        fi
      done
      
      source_path=${codename_path}/main/source
      echo "generating Sources for ${source_path}"
      if [ -e ${source_path}/Sources ]; then
	      rm -rf ${source_path}/Sources
      fi
      # apt-ftparchive sources pool > ${source_path}/Sources
      dpkg-scansources pool > ${source_path}/Sources

      echo "generating Release file"
      if [ -e ${codename_path}/Release ]; then
        rm -rf ${codename_path}/Release
      fi

      if [ -e ${ppa_root}/conf/${channel}/aptftp.conf ];then
	      conf_file=${ppa_root}/conf/${channel}/aptftp.conf
      fi

      apt-ftparchive release -c ${conf_file} ${codename_path} > ${codename_path}/Release
      
      echo "sign Release file"
      if [ -e ${codename_path}/InRelease ]; then
        rm -rf ${codename_path}/InRelease
      fi
      # gpg --clearsign --batch --yes --passphrase "jideos123" -o ${codename_path}/InRelease ${codename_path}/Release
      gpg --clear-sign --batch --yes --passphrase "jideos123" -o ${codename_path}/InRelease ${codename_path}/Release
      
      echo "generating Release.gpg file"
      if [ -e ${codename_path}/Release.gpg ]; then
        rm -rf ${codename_path}/Release.gpg
      fi
      # gpg -abs --batch --yes --passphrase "jideos123" -o ${codename_path}/Release.gpg ${codename_path}/Release
      gpg -ac --batch --yes --passphrase "jideos123" -o ${codename_path}/Release.gpg ${codename_path}/Release
      
      remote_path=s3://ppa.ai.jideos.com${channel_path#${ppa_root}}
      aws s3 sync ${channel_path} ${remote_path} --delete

    else
      echo "${codename_path} not exist" 1>&2
      exit 1
    fi
  done
done
