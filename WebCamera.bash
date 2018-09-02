#!/bin/bash
dir=$(cd $(dirname $0) && pwd)
com=$(basename $0 .bash)
name=${com}

#最初の引数が0〜9ならば，インスタンス名を変更するオプションを追加する．
#「README.モジュール名」というファイルがあることが前提．
case "$1" in
0|1|2|3|4|5|6|7|8|9)
    cat=$(sed -ne '/^Category:/s/.*: *//p' ${dir}/README.${com})
    mod=$(sed -ne '/^Module Name:/s/.*: *//p' ${dir}/README.${com})
    name=${com}$1
    opt="-o ${cat}.${mod}.instance_name:${name}"
    shift
    ;;
esac

gnome-terminal --working-directory=${dir} \
  --command="bash -c 'echo -ne \"\e]0;${name} $*\a\"; build/src/${com}Comp ${opt} $*'"
