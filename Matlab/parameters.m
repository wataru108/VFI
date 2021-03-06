%=============================================================================
%>
%> @file parameters.m
%>
%> @brief File containing parameters class definition.
%>
%> @author Eric M. Aldrich \n
%>         ealdrich@ucsc.edu
%>
%> @version 1.0
%>
%> @date 23 Oct 2012
%>
%> @copyright Copyright Eric M. Aldrich 2012 \n
%>            Distributed under the Boost Software License, Version 1.0
%>            (See accompanying file LICENSE_1_0.txt or copy at \n
%>            http://www.boost.org/LICENSE_1_0.txt)
%>
%=============================================================================

%=============================================================================
%>
%> @brief Object to store parameter values for VFI problem.
%>
%> @details This object stores parameter values for the VFI problem. This
%> `load' method loads parameter values from a text file. The input file must
%> have 13 lines, each line beginning with a parameter value, followed by a
%> comma and a character string describing the parameter. The order of the
%> parameters must correspond to the order in the parameters class
%> description.
%>
%=============================================================================
classdef parameters<handle
    properties
        eta
        beta
        alpha
        delta
        mu
        rho
        sigma
        lambda
        nk
        nz
        tol
    end
    methods
        function load(obj, fileName)
            nParam = 13;
            params = textread(fileName, '%s', 'delimiter', ',');
            obj.eta = str2num(char(params(1)));
            obj.beta = str2num(char(params(3)));
            obj.alpha = str2num(char(params(5)));
            obj.delta = str2num(char(params(7)));
            obj.mu = str2num(char(params(9)));
            obj.rho = str2num(char(params(11)));
            obj.sigma = str2num(char(params(13)));
            obj.lambda = str2num(char(params(15)));
            obj.nk = str2num(char(params(17)));
            obj.nz = str2num(char(params(19)));
            obj.tol = str2num(char(params(21)));
        end
    end
end
        
