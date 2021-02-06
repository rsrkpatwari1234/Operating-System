    case "$1" in 
        *tar*|*tgz|*tbz2)
            tar -xf $1;;
        *rar)
            unrar e $1;;
        *zip)
            unzip $1;;
        *bz2)
            bunzip2 $1;;
        *Z)
            uncompress $1;;
        *gz)
            gunzip $1;;
        *7z)
            7za e $1;;
        *)
            echo Unknown file format: cannot extract;;
    esac