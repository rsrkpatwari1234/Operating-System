mkdir 1.b.files.out
for file in 1.b.files/*.txt
do 
    cat $file|sort -nr>1.b.files.out/${file##*/}
done
sort -mnr 1.b.files.out/*.txt>1.b.out.txt
