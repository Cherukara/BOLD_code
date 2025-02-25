% xSigmoidMapping.m
%
% Testing out the idea of taking measurements of R2' and DBV, and then mapping
% their ratio to values of OEF (probably using a sigmoid function). Script
% outline derived from Figure_R2pScatter.m
%
% MT Cherukara
% 16 April 2019
%
% Actively used as of 2019-04-16
%
% CHANGELOG:
%
% 2019-07-29 (MTC). Sorted things out so that we could just do single set of
%       random-grid OEF and DBV values
%
% 2019-06-24 (MTC). This version is used for sets of 5 results, with DBV values
%       of 1, 3, 5, 7, and 9% respectively. Added functionality for random-grid
%       variant


clear;
close all;
setFigureDefaults;

plot_orig = 0;      % for plotting the tan shape
plot_rand = 1;      % for random plots

% are we using a random grid of OEF and DBV values? If so, paramPairs = 1;
paramPairs = 5;

% % define the five datasets we want
% set1 = 636;
% sets = set1:set1+4;

kappa = 0.43;

% define any number of datasets
sets = 752;

%% Find directories, and load ground truth data and stuff
% Data directory
resdir = '/Users/mattcher/Documents/DPhil/Data/Fabber_ModelFits/';

if paramPairs == 0
    
    % Ground truth
    % OEFvals = linspace(0.21,0.70,no);
    OEFvals = 0.01:0.01:1;
    DBVvals = 0.01:0.02:0.09;
    % DBVvals = 0:0.01:0.09;
    % DBVvals = [0.05,0.05];

else
    
    load(['../Matlab_VesselSim/Sim_OEF_DBV_pairs_',num2str(paramPairs),'.mat']);
    OEFvals = OEFvals(:)';
    DBVvals = DBVvals(:)';
end

no = length(OEFvals);
nd = length(sets);
% mo = 80.*max(OEFvals);
mo = 100;


% Pre-allocate (remove the nd dimension for the 5-DBV analysis)
estR2p = zeros(length(sets),no);
estDBV = zeros(length(sets),no);
estOEF = zeros(length(sets),no);


%% Loop through Datasets
for ss = 1:length(sets)
    
    % Figure out the results directory we want to load from
    fdname = dir([resdir,'fabber_',num2str(sets(ss)),'_*']);
    fabdir = strcat(resdir,fdname.name,'/');
    
    % Load the data
    volR = LoadSlice([fabdir,'mean_R2p.nii.gz'],1);
    volV = LoadSlice([fabdir,'mean_DBV.nii.gz'],1);
    volO = LoadSlice([fabdir,'mean_OEF.nii.gz'],1);
    
    if paramPairs == 0

        % Average
        vecR = mean(volR,1);
        vecV = mean(volV,1);
       
    else
        
        % Vectorize
        vecR = volR(:);
        vecV = volV(:);
%         vecOEF = volO(:);
        
    end
    
    % Calculate Ratio
    vecOEF = kappa.*vecR./(3.55.*vecV);
    
    % Store Results
    estR2p(ss,:) = vecR;
    estDBV(ss,:) = vecV;
    estOEF(ss,:) = vecOEF;
        
   
end % for setnum = ....
   
% sname = strcat('Fabber_Results_',num2str(set1),'.mat');
% save(sname,'estR2p','estDBV');


%% Plotting original

% Plot the other way around
if plot_orig

    figure; box on; hold on; axis square; grid on;
    for ff = 1:length(sets)   
%         scatter(estOEF(ff,:),100*OEFvals,[],'filled');
        scatter(100*OEFvals,estOEF(ff,:),[],'filled');
    %     plot(estOEF(ff,:),yy(ff,:),'--','Color',defColour(ff));
    end
    
    % unity line
    plot([0,100],[0,100],'k-','LineWidth',1);

%     legend('DBV = 1%','DBV = 3%','DBV = 5%','DBV = 7%','DBV = 9%','Location','SouthEast')
%     ylabel('Ratio R2'' / DBV');
    xlabel('Simulated OEF (%)');
    ylabel('Estimated OEF (%)');
    ylim([0,100]);
    xlim([0,100]);
end


%% Plot rands


if plot_rand
    %% Plotting R2' esimates only
% %     trueOEF = repmat(OEFvals,nd,1);
% %     trueDBV = repmat(DBVvals',1,no);
% 
% %     evns = 1:1:no;
% 
%     trueR2p = 355.*OEFvals.*DBVvals;
%     mR = 35;
% %     trueR2p = 355.*trueOEF.*trueDBV;
% %     mR = 0.6*max(trueR2p(:));
% 
%     figure; box on; hold on; axis square; grid on;
%     for ff = 1:length(sets)
%         scatter(trueR2p,estR2p(ff,:),[],'filled');
%     end
% 
%     % plot unity line
%     plot([0,mR],[0,mR],'k-','LineWidth',1);
% 
%     % legend('DBV = 1%','DBV = 3%','DBV = 5%','DBV = 7%','DBV = 9%','Location','NorthWest')
%     xlabel('True R2''');
%     ylabel('Estimated R2''');
%     axis([0,mR,0,mR]);
    
    
    %% Plot DBV estimates
%     mD = 0.1;
%     
%     figure; box on; hold on; axis square; grid on;
%     for ff = 1:length(sets)
%         scatter(DBVvals,estDBV(ff,:),[],'filled');
%     end
% 
%     % plot unity line
%     plot([0,mD],[0,mD],'k-','LineWidth',1);
% 
%     % legend('DBV = 1%','DBV = 3%','DBV = 5%','DBV = 7%','DBV = 9%','Location','NorthWest')
%     xlabel('True DBV (%)');
%     ylabel('Estimated DBV (%)');
%     axis([0,mD,0,mD]);
    
    
    %% Plotting original OEF estimates

    figure; box on; hold on; axis square; grid on;
    for ff = 1:nd
%         scatter(trueOEF(ff,:),kappa*estOEF(ff,:)./355,[],'filled');
        scatter(100*OEFvals,estOEF(ff,:),[],'filled');
    end

    % plot unity line
    plot([0,mo],[0,mo],'k-','LineWidth',1);

%     legend('DBV = 1%','DBV = 3%','DBV = 5%','DBV = 7%','DBV = 9%','Location','NorthWest')
    xlabel('True OEF (%)');
    ylabel('Estimated OEF (%)');
    axis([0,mo,0,mo]);

    yticks([0:20:100]);
    xticks([0:20:100]);
%     yticks([0:0.2:0.8])
%     yticklabels({'0','20','40','60','80'})
%     xticklabels({'0','20','40','60','80'})

    %% Plotting corrected OEF estimates
    
    % pull out true OEFs
    truOEF = 100*OEFvals;

    % % L model
    % modOEF = 0.106.*tan(0.0138*estR2p./estDBV);

    % 2C model
%     modOEF = 0.212.*tan(0.0125*estR2p./estDBV);
    
    % Kappa 2C
%     modOEF = 0.145.*tan(0.00477*estR2p./estDBV);
%     modOEF = 15.1.*tan((0.215*estOEF)-0.17) + 46.2;

    % Sharan
%     modOEF = 31.0.*tan(0.131*estOEF) + 32.9;
    
    % Frechet
%     modOEF = 30.*tan(0.111*estOEF) + 45.7;

    % Lauwers
    modOEF = 12.9.*tan(0.186*estOEF) + 22.0;
    
    badVec = (modOEF > 100) + (modOEF < 0) + (estOEF > 25);
    
    modOEF(badVec > 0.5) = [];
    truOEF(badVec > 0.5) = [];

    figure; box on; hold on; axis square; grid on;
    scatter(truOEF,modOEF,[],'filled');
%     for ff = 1:nd
%         scatter(100*OEFvals,modOEF(ff,:),[],'filled');
%     end
%     legend('DBV = 1%','DBV = 3%','DBV = 5%','DBV = 7%','DBV = 9%','Location','NorthWest')

    % plot unity line
    plot([0,mo],[0,mo],'k-','LineWidth',1);

    xlabel('True OEF (%)');
    ylabel('Estimated OEF (%)');
%     axis([0,mo,0,mo]);

    yticks([0:20:100]);
    xticks([0:20:100]);
%     yticks([0:0.2:0.8])
%     yticklabels({'0','20','40','60','80'})
%     xticklabels({'0','20','40','60','80'})
end