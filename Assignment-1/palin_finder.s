.global _start
_start:
	LDR R0, =input 			//loads the word into memory
	MOV R8, #0 				//initiate the counter for calculating the length of the word
	LOOP: 
		LDRB R1, [R0] 		//read the first byte
		CMP R1, #0 			//check if we have reach the end
		BEQ check_length 	//if the end is reach we need to check if the word is longer than 3 bytes (has to me more than 2 letter (2 bytes) and the last byte which is the newline)
		ADD R8, R8, #1 		//increment the counter
		CMP R1, #97 		//if we have lowercase letters we change them to uppercase
		SUBGE R1, R1, #32   //if the letter is lowercase we cast it to uppercase by removing 32 
		STRB R1, [R0]		//store the updated value
		ADD R0, R0, #1 		//if we are not the end of the word we move to the next byte 
		B LOOP				
	
check_length:
	CMP R8, #3 				//checks if the word is bigger than 3 bytes  
	BGE check_palindrome 	// if it is equal or more we move on to the palindrome check
	B palindrome_not_found 	//if not we can say its not a palindrome without the check
		
check_palindrome:
	// Here you could check whether input is a palindrome or not
	SUB R0, R0, R8 			//Go back to the first letter in our word we want to check
	SUB R8, R8, #1 			//the index of the last letter in our word 
	MOV R3, R0 				//creating the offset
	ADD R3, R3, R8 			//adding the offset amount 
	LDRB R1, [R0] 			//read the first byte of the word
	LDRB R2, [R3] 			//read the last byte of the word 
	
	looping_over_word: 
		CMP R1, #0 	  			//checks if we have reach the end
		BEQ palindrome_found 	//if we have reached the end without going to 'palindrone not found' we know it is a palindrome
		CMP R1, #32				//checks if we have hit a space
		BEQ remove_space_1 		//if we have hit a space we branch to the remove method
		CMP R2, #32				//checks if the other side has encountered a space too
		BEQ remove_space_2		//branch to the method for removing a space if we are on space
		CMP R1, R2				//after the spaces are removed we check if the letter match
		BNE palindrome_not_found //if not we go to palindrome not found
		ADD R0, R0, #1 			//move on to next letter 
		SUB R3, R3, #1 			//changing the index of the last letter
		LDRB R1, [R0] 			//read the first byte of the word
		LDRB R2, [R3] 			//read the last byte of the word 
		B looping_over_word

remove_space_1:
	ADD R0, R0, #1 		//move to next letter	
	LDRB R1, [R0] 		//Load the next letter
	B looping_over_word //go back to the loop

remove_space_2:
	SUB R3, R3, #1	//get index of the next letter, since we are moving from last letter we have to sub not add like the other method
	LDRB R2, [R3]	//load the next letter
	B looping_over_word //go back to loop
		
palindrome_found:
	// Switch on only the 5 rightmost LEDs
	MOV R5, #0b0000011111 	// turn on the 5 lights to the right
	LDR R6, =0xFF200000 	// base address of LED lights
	STR R5, [R6] 			//sending the binary for the five lights to the adress of the lights

	// Write 'Palindrome detected' to UART
	LDR R0, =successtext	
	LDR R1, =0xFF201000 	// base address of JTAG UART
	LDRB R3, [R0] 			//read a singel byte
	B write_to_screen
	


palindrome_not_found:
	// Switch on only the 5 leftmost LEDs
	MOV R5, #0b1111100000 	// turn on the 5 lights to the left
	LDR R6, =0xFF200000   	// base address of LED lights
	STR R5, [R6] 		  	// sending the binary for the five lights to the address of the lights

	// Write 'Not a palindrome' to UART
	LDR R0, =failtext
	LDR R1, =0xFF201000 	// base address of JTAG UART
	LDRB R3, [R0] 	 		//read a singel bite 
	B write_to_screen
	
write_to_screen:
	CMP R3, #0 				//checks if we have reached the end of the word
	BEQ exit				//if we have reached the end stop the loop
	STR  R3, [R1]   		//send the byte to the base address of JTSAG UART
	ADD  R0, R0, #1 		//move to next character in memory
	LDRB R3, [R0] 			//read a singel byte
	B write_to_screen

exit:
	// Branch here for exit
	b exit

.section .data
.align
	input: .asciz "level"
	//input: .asciz "8448"
    //input: .asciz "Kayk"
    //input: .asciz "mlsk on no pets"
    //input: .asciz "Ne ver    od d o r even"
	successtext: .asciz "Palindrome detected\n"
	failtext: .asciz "Not a palindrome\n"
	

	
	
	