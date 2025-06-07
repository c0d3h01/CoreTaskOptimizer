#!/system/bin/sh
MODDIR="${0%/*}"

wait_until_login() {
  until [ "$(getprop sys.boot_completed)" -eq 1 ]; do
    sleep 1
  done
}
wait_until_login

sleep 30 && mkdir -p "$MODPATH/logs" &&
"$MODDIR/libs/task_optimizer" 2>/dev/null
exit 0
