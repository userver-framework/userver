#!/usr/bin/bash

_uctl_commands=(
  'dnsclient reload-hosts'
  'dnsclient flush-cache'
  'dnsclient flush-cache-full'
  'log-dynamic-debug list'
  'log-dynamic-debug force-on'
  'log-dynamic-debug force-off'
  'log-dynamic-debug set-default'
  'log-level get'
  'log-level set trace'
  'log-level set debug'
  'log-level set info'
  'log-level set warning'
  'log-level set error'
  'log-level set none'
  'log-level set default'
  'on-logrotate'
  'access-top'
)

_uctl_log_dynamic_debug_commands=(
  'log-dynamic-debug force-on'
  'log-dynamic-debug force-off'
  'log-dynamic-debug set-default'
)

_uctl_complete()
{
  if [ "${#COMP_WORDS[@]}" != "$((1+${COMP_CWORD}))" ]; then
    return
  fi

  cmdline=$(echo ${COMP_LINE} | sed 's/^[^ ]*//;s/^[^ ]* //')

  guess=()
  for i in ${!_uctl_commands[@]}; do
    cmd=${_uctl_commands[$i]}
    if echo $cmd | grep ^"$cmdline" -q; then
      arr=($cmd)
      guess+=(${arr[$COMP_CWORD-1]})
    fi
  done

  log_lines=""
  for i in ${!_uctl_log_dynamic_debug_commands[@]}; do
    cmd=${_uctl_log_dynamic_debug_commands[$i]}
    if echo $cmdline | grep ^"$cmd" -q; then
      cache_path_dir="$HOME/.cache/uctl"
      cache_path="$cache_path_dir/log_lines.txt"
      if [ ! -f $cache_path ] || [ $(find $cache_path -mmin +60 -type f -print) ]; then
        mkdir -p $cache_path_dir
        $(/usr/bin/${COMP_WORDS[0]} log-dynamic-debug list > $cache_path)
      fi

      log_lines=$(cat $cache_path | cut -f1 | sed -z 's/\n/ /g')
      break
    fi
  done

  COMPREPLY=($(compgen -W "${guess[*]} $log_lines" "${COMP_WORDS[${COMP_CWORD}]}"))
}

complete -F _uctl_complete uctl
