mbpm_filter_result()
{
    while read CurrentWord; do
        if [[ $CurrentWord =~ ^$1 ]]; then
            echo $CurrentWord
        fi
    done
}
mbpm_completion()
{
    declare -i ArgumentCount=0
    for String in ${COMP_LINE}; do
        if [[ ! $String =~ ^- ]]; then
            let ArgumentCount+=1;
        fi
    done
    if ((ArgumentCount <= 2));  then
        mbpm commands | mbpm_filter_result $2
        return 0
    fi
    if [[ $2 =~ / ]]; then
        compgen -o dirnames $2
        return 0
    fi
    mbpm packets --allinstalled --name | mbpm_filter_result $2
    return 0
}
