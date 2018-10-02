% predict y = a * e^(k*t) + c 
% 

times = (0:10:90)';
temperature = [
   96.000
   69.889
   59.273
   54.957
   53.202
   52.489
   52.199
   52.081
   52.033
   52.013
];

plot(times, y);

% for those data, y = 44 * e.^(-0.09*times) + 52