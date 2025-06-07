# MMT Extended Config Script.
# Construct your own list here
# REPLACE=""

# Permissions
set_permissions() {
    set_perm_recursive "$MODPATH" 0 0 0755 0644
    set_perm_recursive "$MODPATH/libs" 0 0 0777 0755
}

SKIPUNZIP=1
unzip -qjo "$ZIPFILE" 'common/functions.sh' -d $TMPDIR >&2
. $TMPDIR/functions.sh
