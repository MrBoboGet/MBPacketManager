PS1="\[\e]0;\u@\h: \w\a\]\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w \n\$ \[\033[00m\]"
PS2=">"
PS4="+"
alias ls="ls --color=auto"
alias grep="grep --color=auto"

declare -A ExtensionHandlers
ExtensionHandlers=(["png"]="ffplay "  ["jpg"]="ffplay " ["mp4"]="ffplay " ["mp3"]="ffplay ")
command_not_found_handle()
{
    CommandToExecute=""
    Extension=${1##*.}
    if [[ ${ExtensionHandlers[$Extension]+asd} ]]; then
        CommandToExecute=${ExtensionHandlers[$Extension]}
    fi
    if [[ $CommandToExecute != "" ]]; then
        $CommandToExecute $1 > /dev/null 2>&1
        return $?
    else
        echo "Command ${1} not found. mbpm install ${1} will most likely not work B)"
        return 1
    fi
}
#TODO: fix 
