% (C) Alexander Sorokin, Daria Sorokina, 2009
% License: New BSD.

function make_effect_plot(xvalues, xcounts, values, plot_title)

clf
axes('position',[0.1 0.2,0.8 0.7])
hold on

%lc = [198 198 198]/255;
%lw = 3;

n = numel(xvalues);
hasMV = (xvalues(n) == 0);
if(hasMV)
	h = plot(xvalues(1:n-1), values(1:n-1));
else
	h = plot(xvalues, values);
end
set(h,'LineWidth', 5);
set(h,'Color', [127 127 127]/255)
if(hasMV)
  mv_off = 1;
  if n > 2
	  mv_off = (xvalues(n-1) - xvalues(1)) / 10;
	end
  xvalues(n) = xvalues(n-1) + mv_off;
	mv = plot(xvalues(n), values(n), "*");
	set(mv,'LineWidth', 5);
	set(mv,'Color', [127 127 127]/255)
end
	
xLegend=cell(1, numel(xvalues));
m = n;
if(hasMV)
	m=n-1;
end
for iX=1:m
    if xcounts(iX) == 1
        s = num2str(xvalues(iX));
    else
        s = [num2str(xvalues(iX)) ' (x ' num2str(xcounts(iX)) ')'];
    end
    xLegend{iX} = s;
end
if(hasMV)
	if xcounts(n) == 1
		s = '?';
	else
		s = ['? (x ' num2str(xcounts(n)) ')'];
	end
	xLegend{n} = s;
end
set(gca, 'XTick', xvalues, 'XTickLabel', xLegend)
rotateticklabel(gca,45);
title(fix_title(plot_title)); 