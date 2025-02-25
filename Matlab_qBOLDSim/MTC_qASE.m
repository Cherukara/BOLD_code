% MTC_qASE.m
%
% Quantitative Asymmetric Spin Echo Sequence Simulation. Generates data to
% be used in Bayesian inference on parameters (MTC_Asymmetric_Bayes.m and
% others). Based on MTC_qBOLD.m 
%
% 
%       Copyright (C) University of Oxford, 2016-2019
%
% 
% Created by MT Cherukara, 17 May 2016
%
% CHANGELOG:
%
% 2018-10-10 (MTC). Added the option to specify which model to use, and whether
%       to include T1 effects (including FLAIR), as PARAMS. Added genParams.m as
%       a function to generate the PARAMS structure, rather than hard-coding it
%       in this script
%
% 2018-04-05 (MTC). Added the option to set the critical time (TC) manually as a
%       pair of parameters. Also changed MTC_ASE_tissue.m accordingly.
%
% 2018-01-12 (MTC). Change the way the noise standard deviation params.sig
%       is calculated and applied, so that it actually results in the SNR
%       that we want. Also some cleanup.
%
% 2017-08-07 (MTC). Added a call to MTC_qASE_model, rather than repeating
%       its contents here, so that the whole thing is more modular. Removed
%       the automatic saving of the plot, as it's kind of unnecessary
%
% 2017-04-04 (MTC). Modified the plotting parameters, removed a bunch of
%       unnecessary variables from the params struct.
%
% 2016-11-08 (MTC). Changed tau range to go up to 64 ms, changed some
%       other constants, added saving out of data sampled at key points,
%       with noise added in too.
% 
% 2016-05-27 (MTC). Changed the way the compartments are added together,
%       which was apparently causing problems (I don't know why).

clear; 
% close all;

setFigureDefaults;

plot_fig = 1;       
save_data = 0;      % set to 1 in order to save out ASE data


%% Model Parameters

% Create a parameter structure
params = genParams;

% Assign specific parameters

params.B0 = 3;

% Scan
params.TE   = 0.084;        % s         - echo time
params.TR   = 3;
params.TI   = 1.21;

% Physiology
params.lam0 = 0.05;         % no units  - ISF/CSF signal contribution
params.zeta = 0.03;         % no units  - deoxygenated blood volume
params.OEF  = 0.40;         % no units  - oxygen extraction fraction

params.dF = 5;

% Simulation
params.model  = 'Asymp';     % STRING    - model type: 'Full','Asymp','Phenom','Kiselev'
params.contr  = 'OEF';      % STRING    - contrast source: 'OEF','R2p','dHb',...
params.incT1  = 1;          % BOOL      - should T1 differences be considered?
params.incT2  = 1;          % BOOL      - should T2 differences be considered?
params.incIV  = 1;          % BOOL      - should blood compartment be included?

% Scaling
% params.SR   = 0.47;        % no units  - scaling factor for R2'
% params.Voff = 0.47;
% params.sgeo = 0.3;
% params.beta = 1;
% params.alpha = 0;
% params.eta  = 0.05;

% noise
params.SNR = 100;


%% Compute Model

% define tau values that we want to simulate
% tau = (-16:8:64)/1000; % for testing
tau = linspace(-0.028,0.064,1000); % for visualising


np = length(tau);

% call MTC_qASE_model
[S_total,params] = qASE_model(tau,params.TE,params);

% Normalize to the spin-echo
SEind = find(tau > -1e-9,1);
S_total = S_total./S_total(SEind);
% S_total = S_total./mean(S_total(SEind-1:SEind+1));      % alternative normalization method

%% Optionally add noise 
S_sample = S_total + (randn(1,np)./params.SNR);
S_sample(S_sample < 0) = 0;

% calculate maximum data standard deviaton
params.sig = min(S_sample)/params.SNR;


%% plot figure
if plot_fig
    
    % create a figure
    figure(1); hold on; box on; 
    
    % plot the signal
    S_log = log(S_total);
    l.s = plot(1000*tau,S_log,'-');
%     ylim([-0.07,0]);
    xlim([(1000*min(tau)), (1000*max(tau))]);
    
    % labels on axes
    xlabel('Spin Echo Displacement \tau (ms)');
    ylabel('Log (Signal)');

end % if plot_fig

% Save Data
if save_data
    % Check how many datasets have been saved with the same date
    dat_dir = '/Users/mattcher/Documents/DPhil/Code/Matlab_Bayesian_MCMC/';
    dat_title1 = strcat('ASE_signal_',date,'_');
    dat_list = dir(strcat(dat_dir,dat_title1,'*'));
    fn = length(dat_list) + 1;
    
    % Assign the correct title
    dat_title = strcat(dat_title1,num2str(fn));
    
    % pull out values of TE and tau
    T_sample = tau;
    
    if length(params.TE) ~= length(tau)
        TE_sample(1:length(tau)) = params.TE;
    else
        TE_sample = params.TE;
    end
    
    % Save the data out
    save(dat_title,'T_sample','S_sample','TE_sample','params','S_total');
end % if save_data
    

%% Further Model Analyses - 18 July 2018

% % calculate R2p
% params.R2p = params.dw.*params.zeta;
% 
% 
% % define short tau regime
% shrts = abs(tau) < (1.7/params.dw);
% T_shrt = tau(  shrts);
% S_shrt = S_log(shrts);
% 
% % define long tau regime
% longs = tau > (1.7/params.dw);
% T_long = tau(  longs);
% S_long = S_log(longs);
% 
% % define "very long tau" regime
% vlongs = tau > 0.0395;
% T_vlong = tau(  vlongs);
% S_vlong = S_log(vlongs);
% 
% % define transition regime (within 5 ms of Tc) 
% trns = abs( tau - (1.7/params.dw) ) < 0.005;
% T_mid = tau(  trns);
% S_mid = S_log(trns);
% 
% 
% % solve for A in "new" exponential long tau model
% A_long = ( params.zeta - S_long - (params.R2p*T_long) ) ./ (T_long.^2);
% 
% % % plot a figure
% % figure; hold on; box on;
% % plot(1000*T_long,A_long,'k-');
% % 
% % xlabel('tau (ms)');
% % ylabel('A');
