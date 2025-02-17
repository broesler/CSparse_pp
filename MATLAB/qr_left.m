function [V,Beta,R] = qr_left (A)
%QR_LEFT left-looking Householder QR factorization.
% Example:
%  [V,Beta,R] = qr_left (A)
% See also: cs_demo

% CSparse, Copyright (c) 2006-2022, Timothy A. Davis. All Rights Reserved.
% SPDX-License-Identifier: LGPL-2.1+

[m n] = size (A) ;
V = zeros (m,n) ;
Beta = zeros (1,n) ;
R = zeros (m,n) ;
for k = 1:n
    x = A (:,k) ;
    for i = 1:k-1
        v = V (i:m,i) ;
        beta = Beta (i) ;
        x (i:m) = x (i:m) - v * (beta * (v' * x (i:m))) ;
    end
    [v,beta] = gallery ('house', x (k:m)) ;
    V (k:m,k) = v ;
    Beta (k) = beta ;
    R (1:(k-1),k) = x (1:(k-1)) ;
    % R (k,k) = s ;
    if k == 1
        warning('FIXME: R(k,k) = -sign(x(1)) * norm(x) ;  % FIXME wrong value.');
    end
    R (k,k) = -sign(x(1)) * norm(x);  % FIXME wrong value since v(1) != 1
end
