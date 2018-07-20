%Initialize
% hrealterm=actxserver('realterm.realtermintf');
% hrealterm.baud=250000;
% hrealterm.flowcontrol=0; %no handshaking currently
% hrealterm.Port='10';
% hrealterm.PortOpen=1; %open the comm port

% variables
image_size = 71337;
packet_acknowledged = 0;
bytes_received = 0;
fsize = 0;
fsizenew = 0;
command = zeros(4);
data = zeros(248);

if exist('C:\temp\cap2.bin', 'file')
  readfile = fopen('C:\temp\cap2.bin','r'); %open capture file for read
  frewind(readfile);
else
  % File does not exist. Create file.
  fclose(fopen('C:\temp\cap2.bin', 'w'));
  readfile = fopen('C:\temp\cap2.bin','r'); %open capture file for read
end

if readfile < 3
    errordlg('Data capture file not opened. Check existence of file at C:\temp\cap.bin, permissions and whether the file is opneed in another program.',...
        'Invalid Input','modal')
    uicontrol(hObject)
    return
end

%Make sure realterm uses cap.bin as capture file
hrealterm.CaptureFile='C:\temp\cap2.bin';
hrealterm.CaptureAsHex = 0;
invoke(hrealterm,'startcapture'); %start capture on realterm

%Open FPGA Image File
fid = fopen('FPGAimage.bin','r');

checksum = uint64(0);
for i=1:17835
    checksum = checksum + uint64(fread(fid,1,'uint32',0,'a'));
    %read file (open file, read only one emelent, size of each row(precision), number of bytes to skip, 'ieee-le' little endian, 64 bit long data type);
end

frewind(fid); %sets the file position indicator to the beginning of the file
%fclose(fid);
b = mod(checksum, 4294967296) %modulus, should be zero ? TODO?
checksum = b;

%Create Progress Bar
h = waitbar(0,'Upload');

%Pause heartbeat monitor
invoke(hrealterm, 'putstring', hex2dec('23'), 2);

%sample uart feedback file size
fsize = dir('C:\temp\cap2.bin'); %new filesize info

%Begin Upload Command Packet
packet_acknowledged = 0;
bytes_received = 0;
invoke(hrealterm, 'putstring', hex2dec('85'), 2);
invoke(hrealterm, 'putstring', 3, 2);
invoke(hrealterm, 'putstring', hex2dec('FB'), 2);
invoke(hrealterm, 'putstring', hex2dec('9A'), 2);
invoke(hrealterm, 'putstring', hex2dec('BB'), 2);
pause(0.5);
invoke(hrealterm, 'putstring', hex2dec('85'), 2);
invoke(hrealterm, 'putstring', 1, 2);
invoke(hrealterm, 'putstring', hex2dec('61'), 2);
pause(0.1);
invoke(hrealterm, 'putstring', hex2dec('85'), 2);
invoke(hrealterm, 'putstring', 1, 2);
invoke(hrealterm, 'putstring', hex2dec('61'), 2);
pause(0.1);
%poll for new data after the command sequence
tic;
while ((toc < 1) && bytes_received == 0)
    fsizenew = dir('C:\temp\cap2.bin'); %new filesize info
    pause(0.05);
    if (fsizenew.bytes > (fsize.bytes + 4))
       bytes_received = 1;
    end
end

if (bytes_received == 1)
    command = fread(readfile,4,'uint8')';
    if (command(1) == 218 && command(2) == 122 && command(3) == 4) %look for 0xDA 0x7A 0x04
        data = fread(readfile,3,'uint8')'; %read the next three
        if (data(3) == 1)                  %TODO unsure why its a one ?
            packet_acknowledged = 1;
        end

    end
    fsize.bytes = fsizenew.bytes;
end
if (packet_acknowledged == 0) %close for comms error 
    disp('transmission error')
    %invoke(hrealterm,'stopcapture');
    close(h);
    fclose(fid);
    fclose(readfile);
    return
end



for j=1:287
    packet_acknowledged = 0;
    bytes_received = 0;
    fsize = dir('C:\temp\cap2.bin'); %new filesize info
    invoke(hrealterm, 'putstring', hex2dec('85'), 2);
    invoke(hrealterm, 'putstring', 251, 2);
    invoke(hrealterm, 'putstring', hex2dec('FB'), 2);
    invoke(hrealterm, 'putstring', hex2dec('9A'), 2);
    invoke(hrealterm, 'putstring', 62, 2);
    for i=1:248
        data = fread(fid,1);
        invoke(hrealterm, 'putstring', data, 2);
    end
    pause(0.2);
    tic
    while ((toc < 1) && bytes_received == 0)
        invoke(hrealterm, 'putstring', hex2dec('85'), 2);
        invoke(hrealterm, 'putstring', 1, 2);
        invoke(hrealterm, 'putstring', hex2dec('61'), 2);
        pause(0.1);
        fsizenew = dir('C:\temp\cap2.bin'); %new filesize info
        if (fsizenew.bytes > (fsize.bytes + 4))
           bytes_received = 1;
        end
    end
    if (bytes_received == 1)
        command = fread(readfile,4,'uint8')';
        if (command(1) == 218 && command(2) == 122 && command(3) == 4)
            data = fread(readfile,3,'uint8')';
            if (data(3) == 1)
                packet_acknowledged = 1;
            end

        end
        fsizenew = dir('C:\temp\cap2.bin'); %new filesize info
        fsize = fsizenew;
    end
    if (packet_acknowledged == 1)
        waitbar(j/287,h);
    else
        disp('transmission error')
        %invoke(hrealterm,'stopcapture');
        close(h);
        fclose(fid);
        fclose(readfile);
        return
    end

end

%Last data packet
packet_acknowledged = 0;
bytes_received = 0;
invoke(hrealterm, 'putstring', hex2dec('85'), 2);
invoke(hrealterm, 'putstring', 167, 2);
invoke(hrealterm, 'putstring', hex2dec('FB'), 2);
invoke(hrealterm, 'putstring', hex2dec('9A'), 2);
invoke(hrealterm, 'putstring', 41, 2);
for i=1:161

    invoke(hrealterm, 'putstring', fread(fid,1), 2);

end
invoke(hrealterm, 'putstring', hex2dec('FF'), 2);
invoke(hrealterm, 'putstring', hex2dec('FF'), 2);
invoke(hrealterm, 'putstring', hex2dec('FF'), 2);
pause(0.2);
tic
while ((toc < 1) && bytes_received == 0)
    invoke(hrealterm, 'putstring', hex2dec('85'), 2);
    invoke(hrealterm, 'putstring', 1, 2);
    invoke(hrealterm, 'putstring', hex2dec('61'), 2);
    pause(0.1);
    fsizenew = dir('C:\temp\cap2.bin'); %new filesize info
    if (fsizenew.bytes > (fsize.bytes + 4))
       bytes_received = 1;
    end
end
if (bytes_received == 1)
    command = fread(readfile,4,'uint8')';
    if (command(1) == 218 && command(2) == 122 && command(3) == 4)
        data = fread(readfile,3,'uint8')';
        if (data(3) == 1)
            packet_acknowledged = 1;
        end

    end
    fsizenew = dir('C:\temp\cap2.bin'); %new filesize info
    fsize = fsizenew;
end
if (packet_acknowledged == 1)
    waitbar(j/287,h);
else
    disp('transmission error')
    %invoke(hrealterm,'stopcapture');
    close(h);
    fclose(fid);
    fclose(readfile);
    return
end




%Validation command packet
packet_acknowledged = 0;
bytes_received = 0;
invoke(hrealterm, 'putstring', hex2dec('85'), 2);
invoke(hrealterm, 'putstring', 11, 2);
invoke(hrealterm, 'putstring', hex2dec('FB'), 2);
invoke(hrealterm, 'putstring', hex2dec('9A'), 2);
invoke(hrealterm, 'putstring', hex2dec('CC'), 2);
msg = typecast(uint32(checksum),'uint8');
for i=1:4
    invoke(hrealterm, 'putstring', msg(i), 2);
end
msg = typecast(uint32(image_size),'uint8');
for i=1:4
    invoke(hrealterm, 'putstring', msg(i), 2);
end
pause(0.5);
invoke(hrealterm, 'putstring', hex2dec('85'), 2);
invoke(hrealterm, 'putstring', 1, 2);
invoke(hrealterm, 'putstring', hex2dec('61'), 2);
pause(0.1);
invoke(hrealterm, 'putstring', hex2dec('85'), 2);
invoke(hrealterm, 'putstring', 1, 2);
invoke(hrealterm, 'putstring', hex2dec('61'), 2);
pause(0.1);
tic;
while ((toc < 1) && bytes_received == 0)
    fsizenew = dir('C:\temp\cap2.bin'); %new filesize info
    pause(0.05);
    if (fsizenew.bytes > (fsize.bytes + 4))
       bytes_received = 1;
    end
end
if (bytes_received == 1)
    command = fread(readfile,4,'uint8')';
    if (command(1) == 218 && command(2) == 122 && command(3) == 4)
        data = fread(readfile,3,'uint8')';
        if (data(3) == 1)
            packet_acknowledged = 1;
        end

    end
    fsize.bytes = fsizenew.bytes;
end
if (packet_acknowledged == 0)
    disp('transmission error')
    %invoke(hrealterm,'stopcapture');
    close(h);
    fclose(fid);
    fclose(readfile);
    return
end

%invoke(hrealterm,'stopcapture');
close(h);
fclose(fid);
fclose(readfile);

%Close
%invoke(hrealterm,'close');
%delete(hrealterm);
