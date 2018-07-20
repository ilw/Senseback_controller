function en_rec_implant(handles,block,ch_bits,iir,A0,A1,LFP,LP,bits_hp,shift)
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
% i added the shift variable


if ch_bits>15 || ch_bits<0 || iir>15 || iir <0 || A0>15 || A0<0 || ...
        A1>15 || A1<0 ||LFP>15 || LFP<0 || LP>15 || LP<0 || block>7 ...
        || block<0 || bits_hp >31 || bits_hp<0 || shift <0 || shift >9
    disp('Error ch_bits<16, iir<16, A0<16, A1<16, LFP<16,LP<16, block<8, bits_hp<32, shift should be 1 - 9 if using iir');
    return;
end


byte0 = bin2dec(fliplr(dec2bin(   uint8(ch_bits)  ,4)));
byte1 = bin2dec(fliplr(dec2bin(    uint8(iir) + bitshift(uint8(shift),4)   ,8)));
byte2 = bin2dec(fliplr(dec2bin(   bitor(bitshift(uint8(A0),4),uint8(A1))   ,8)));
byte3 = bin2dec(fliplr(dec2bin(   bitor(bitshift(uint8(LFP),4) , uint8(LP))  ,8)));
byte13 = bin2dec(fliplr(dec2bin(  bitshift(uint8(bits_hp),0)   ,5)));
byte8 = bitor( uint8(block)   ,8);


invoke(handles.hrealterm, 'putchar', uint8(133)); %tell TX that theres a packet to send
invoke(handles.hrealterm, 'putchar', uint8(14)); %Tell TX how many bytes are in the packet

write_reg(handles,0,byte0);
write_reg(handles,1,byte1);
write_reg(handles,2,byte2);
write_reg(handles,3,byte3);
write_reg(handles,13,byte13);
write_reg(handles,8,byte8);

invoke(handles.hrealterm, 'putchar', uint8(0));
invoke(handles.hrealterm, 'putchar', uint8(0));


end

