IFS=',' read -a nums<<<"$1$2$3$4$5$6$7$8$9${10}${11}${12}${13}${14}${15}${16}${17}"
len=${#nums[@]}
if ((len>9));then exit;fi 
GCD(){ 
    if [ "$2" -ne 0 ];then 
        GCD "$2" "$(($1%$2))"
    else echo "$1";fi }
gcd=$((nums[0]))
for i in "${nums[@]}"
do
    gcd=$(GCD $gcd ${i##*[+-]})
done
echo "$gcd"    