#!/bin/sh

VERSION=${1}
GITREPO=${2}
OUTPUTDIR=${3}

if [ -z ${VERSION} ] || [ -z {GITREPO} ]; then
  echo "Usage: `basename ${0}` <VERSION> <GITREPO> [<OUTPUTDIR>]"
  exit 1
fi

if [ -z $OUTPUTDIR ] || [ ! -d $OUTPUTDIR ]; then
  OUTPUTDIR="${PWD}"
fi

git archive --format=tar --prefix=kwebkitpart-${VERSION}/ ${GITREPO} | bzip2 -9 > ${OUTPUTDIR}/kwebkitpart-${VERSION}.tar.bz2
