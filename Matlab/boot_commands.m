% script to initiate new app update OTA with communication with the controller


%en_rec_implant(handles,0,0,0,0,0,15,15,0,0);
invoke(handles.hrealterm, 'putstring', ' '); %reset sequence
pause(10); %wait for connection to establish
invoke(handles.hrealterm, 'putstring', 'B'); %Initialise boot
pause(1);
invoke(handles.hrealterm, 'putstring', 'B045'); %send file size
pause(1);
invoke(handles.hrealterm, 'putstring', 'B'); %check size is valid
pause(1);
invoke(handles.hrealterm, 'putstring', 'B'); %check size is valid
% invoke(handles.hrealterm, 'putstring', ''); %start extended packet send
% invoke(handles.hrealterm, 'putchar', uint8(4)); %send packet length TODO get exact size of this o.O
