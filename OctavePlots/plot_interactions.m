% (C) Alexander Sorokin, Daria Sorokina
% License: New BSD.

folder='./';

files=dir([folder '*iplot.txt']);
for iF=1:numel(files)

    fn=files(iF).name; %fn='x1.x3.iplot.txt';
    fn2=strrep(fn,'iplot.txt','iplot.dens.txt');
    if ~exist(fullfile(folder, fn2),'file')
        warning(['Missing file ' fn2 ' for file ' fn '. Skipping ' fn '.']);
        continue
    end

    f=fopen(fullfile(folder, fn), 'r');
    s=fgets(f);
    s=fgets(f);
    var1 = strtrim(s(8:end));
    s=fgets(f);
    var2 = strtrim(s(11:end));
    fclose(f);
    [data]=dlmread(fullfile(folder, fn),'\t',5,0);

    xvalues = data(2, 3:end)';
    xcounts = data(1, 3:end)';
    yvalues = data(3:end, 2);
    ycounts = data(3:end, 1);
    values = data(3:end, 3:end);
    if xcounts(1) == 0
        error(['Incorrect data for quantile counts. If your data comes from LRTree, you should be using plot_interactions_lrtree script instead'])
        continue
    end
    [density]=dlmread(fullfile(folder, fn2),'\t',4,0);
    
    make_interaction_plot(xvalues, xcounts, yvalues, ycounts, values, density, var2, fn);

    print(gcf,'-depsc',[folder fn '.eps']);
    %print(gcf,'-djpeg90',[folder fn '.jpg']);

    make_interaction_plot(yvalues, ycounts, xvalues, xcounts, values', density', var1, ['Flipped ' fn]);

    print(gcf,'-depsc',[folder fn '.flipped.eps']);
    %print(gcf,'-djpeg90',[folder fn '.flipped.jpg']);

end