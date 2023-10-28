if [ $# -ne 3 ]; then
    echo "Usage <number of clients> <loop num> <sleep time>"
    exit
fi

gcc -o client gradingclient.c
mkdir -p outputs
rm -f outputs/*

for (( i=1 ; i<=$1 ; i++ )); 
do
    ./client 127.0.0.1 3001 correct_code.c $2 $3 $i > outputs/op$i.txt &
done
wait


cat outputs/*.txt | awk '
    BEGIN{
    	FS=":";
        sum=0;
        total=0;
        thru=0;
    }
    
    {
    	if($1 ~ /throughput/ ){
    		thru=thru+$2;
    	}
    	if($1 ~ /total_time/ ){
    		ti = $2;
    	}
    	if($1 ~ /average/ ){
    		avg = $2;
    	}
        sum=sum+(ti*avg)
        total=total+ti;
        
    }

    END{
        printf("Average time taken = %f ms. Throughput = %f\n", sum/total, thru)
    }
' 


