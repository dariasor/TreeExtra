% (C) Alexander Sorokin, Daria Sorokina
% License: New BSD.

function make_interaction_plot(xvalues, xcounts, yvalues, ycounts, values, density, xlabelstr, plot_title)

clf
hold on
lw_scale = 6/numel(yvalues);
lw_offset = 3;
lc_scale = [198 198 198]/255/numel(yvalues);
lc_offset = [0 0 0];

celln = sum(xcounts)*sum(ycounts);
top = (1/celln) * 10;
bottom = (1/celln) / 10;

n=numel(xvalues);
m=n;
hasMV=(xvalues(n)==0);
if hasMV
	m=n-1;
  mv_off = 1;
  if n > 2
	  mv_off = (xvalues(n-1) - xvalues(1)) / 10;
  end
	xvalues(n) = xvalues(n-1) + mv_off;	
end
for iY = 1:numel(yvalues)
    lw = iY*lw_scale+lw_offset;
    lc = iY*lc_scale+lc_offset;
    handles(iY) = plot(xvalues(1:m), values(iY,1:m),'LineWidth',lw,'Color',lc);
	if hasMV
		plot(xvalues(n), values(iY,n), "*", 'LineWidth',lw,'Color',lc);
	end
    %plot high/low density markers
    for iX = 1:numel(xvalues)
        if density(iY, iX) > top
            plot(xvalues(iX), values(iY, iX), 'og', 'MarkerSize', 12, 'LineWidth', 3);
        end
        if density(iY, iX) < bottom
            plot(xvalues(iX), values(iY, iX), 'or', 'MarkerSize', 12, 'LineWidth', 3);
        end
    end
end
hold off

xLegend = cell(1, numel(xvalues));
for iX = 1:m
    s = num2str(xvalues(iX));
    if xcounts(iX) > 1
        s = [s ' (x' num2str(xcounts(iX)) ')'];
    end
    xLegend{iX} = s;
end
if(hasMV)
	xLegend{n} = '?';
	if xcounts(n) > 1
		xLegend{n} = ['? (x' num2str(xcounts(n))  ')' ];
	end
end	
set(gca,'XTick', xvalues, 'XTickLabel', xLegend );
xlh = xlabel(xlabelstr);
set(xlh, 'FontSize', 20);
rotateticklabel(gca,45);

yLegend = cell(1, numel(yvalues));
for iY = 1:numel(yvalues)
    s = num2str(yvalues(iY));
    if ycounts(iY) > 1
        s = [s ' (x' num2str(ycounts(iY)) ')'];
    end
    yLegend{iY} = s;
end
yn = numel(yvalues);
if yvalues(yn) == 0
	yLegend{yn} = '?'; 
	if ycounts(yn) > 1
		yLegend{yn} = ['? (x' num2str(ycounts(yn))  ')' ]; 
	end
end
	
legend(handles, yLegend,'Location','NorthEastOutside');

title(fix_title(plot_title));