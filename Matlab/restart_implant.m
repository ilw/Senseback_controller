invoke(handles.hrealterm, 'putchar', uint8(133)); %tell TX that theres a packet to send
invoke(handles.hrealterm, 'putchar', uint8(3)); %Tell TX how many bytes are in the packet
invoke(handles.hrealterm, 'putchar', uint8(18)); %flush 
invoke(handles.hrealterm, 'putchar', uint8(53)); 
invoke(handles.hrealterm, 'putchar', uint8(55)); 