program begins:
PROGRAM START
When thread 2 is created:
CONSUMER THREAD CREATED
When myCount changes value:
myCount: <PREVIOUS#> -> <NEW#>
Example:
myCount: 1 -> 2
When myMutex is unlocked:
<THREAD>: myMutex unlocked
Example:
CONSUMER: myMutex unlocked
When myMutex is locked:
<THREAD>: myMutex locked
Example:
CONSUMER: myMutex locked
When myCond1 or myCond2 has pthread_cond_wait() called on it:
<THREAD>: waiting on <CONDITION VAR>
Example:
PRODUCER: waiting on myCond1
When myCond1 or myCond2 has pthread_cond_signal() called on it:
<THREAD>: signaling <CONDITION VAR>
Example:
CONSUMER: signaling myCond1
When your program ends:
PROGRAM END
