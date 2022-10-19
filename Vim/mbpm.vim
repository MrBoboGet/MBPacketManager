function mbpm#AddPluginDir(PluginDir) abort
    let PluginDir = expand(a:PluginDir) . "/"
    let PluginDirectories = []
    let DirContents = readdir(PluginDir)
    for Name in DirContents
        if isdirectory(PluginDir . Name)
            call add(PluginDirectories,PluginDir . Name)
        endif
    endfor
    for Dir in PluginDirectories
        let &rtp .= "," . Dir
    endfor
endfunction
