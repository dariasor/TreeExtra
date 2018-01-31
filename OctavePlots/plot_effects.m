folder='./';

files=dir([folder '*effect.txt']);
for iF = 1:numel(files)

    fn = files(iF).name; 

    data = dlmread([folder fn],'\t',2,0);

    xcounts = data(1:end, 1);
    xvalues = data(1:end, 2);
    values = data(1:end, 3);
    
    make_effect_plot(xvalues, xcounts, values, fn)

    print(gcf,'-depsc',[folder fn '.eps']);
end 