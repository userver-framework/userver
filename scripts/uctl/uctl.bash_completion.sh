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

  COMPREPLY=($(compgen -W "${guess[*]}" "${COMP_WORDS[${COMP_CWORD}]}"))
}

complete -F _uctl_complete uctl
