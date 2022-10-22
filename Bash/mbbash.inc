R"1337xd(SourceDir=${BASH_SOURCE%/*}
for Directory in $(ls $SourceDir); do
    CurrentDir=$SourceDir/$Directory
    if [[ ! -d $CurrentDir ]]; then
        continue
    fi
    for Source in $(ls $CurrentDir); do
        CurrentSource=$CurrentDir/$Source
        if [[ ! $CurrentSource =~ \.sh$ ]]; then
            continue
        fi
        . $CurrentSource
    done
done)1337xd"
