Easiest first point of discussion of the advantages/draw backs of the three languages is that both Java and Python
have storage data structures with built-in ways to handle mutual exclusion and are also thread safe. Python has Queues
and Java has ArrayBlockingQueues. This is separate from C++ where you use pthreads to manually build your own mutual 
exclusion based on mutex locks. 
Java and Python are easier as they hide most of the complexity behind the built-in methods and do much of the mutual
exclusion automatically behind the scenes. C++ is the hardest and easiest to also mess up as you have to do all of that
manually with raw synchronization primitives. It allows the best control and performance but also the easiest screwups.

Overall:
Python and Java hiding most of the complexity makes them better for quick implementations where actual speed doesn't 
matter as much. C++ gives the most control and is also the quickest in terms of runtime, but also is much more 
complicated and takes a lot more work to implement. 

Sidenote: let me know if the documentation doesn't show up for the Java file, I can see it when I have the file open
in Itellij but when I look at it in github or one of the other two IDEs I don't see any documentation. I can see all the
other documentation for the other two languages in github and in their respective IDEs so I don't know if something is 
wrong with the Java one or not.