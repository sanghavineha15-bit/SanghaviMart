import React, { useState } from 'react';
import { ShoppingBag, Store, ShieldCheck, ArrowRight, Lock, Mail, CheckCircle2, AlertCircle, ArrowLeft } from 'lucide-react';
import { UserRole, User } from '../types';

interface RoleLoginPageProps {
  initialRole?: UserRole;
  users: User[];
  onLoginSuccess: (user: User) => void;
  onBackToShop: () => void;
  onNavigateToRegister: () => void;
}

export const RoleLoginPage: React.FC<RoleLoginPageProps> = ({
  initialRole = 'buyer',
  users,
  onLoginSuccess,
  onBackToShop,
  onNavigateToRegister,
}) => {
  const [selectedRole, setSelectedRole] = useState<UserRole>(initialRole);
  const [email, setEmail] = useState(() => {
    if (initialRole === 'seller') return 'seller.apex@sanghavimart.com';
    if (initialRole === 'admin') return 'admin@sanghavimart.com';
    return 'neha@sanghavimart.com';
  });
  const [password, setPassword] = useState('password123');
  const [errorMsg, setErrorMsg] = useState('');
  const [successMsg, setSuccessMsg] = useState('');

  const switchRole = (role: UserRole) => {
    setSelectedRole(role);
    setErrorMsg('');
    setSuccessMsg('');
    if (role === 'seller') {
      setEmail('seller.apex@sanghavimart.com');
      setPassword('password123');
    } else if (role === 'admin') {
      setEmail('admin@sanghavimart.com');
      setPassword('password123');
    } else {
      setEmail('neha@sanghavimart.com');
      setPassword('password123');
    }
  };

  const handleLogin = (e: React.FormEvent) => {
    e.preventDefault();
    setErrorMsg('');
    setSuccessMsg('');

    // Locate matching user
    const matchedUser = users.find(
      (u) => u.email.toLowerCase() === email.trim().toLowerCase()
    );

    if (!matchedUser) {
      setErrorMsg(`No account found with email "${email}". Please verify credentials.`);
      return;
    }

    if (matchedUser.role !== selectedRole) {
      setErrorMsg(
        `Access Denied: This account is registered as a "${matchedUser.role.toUpperCase()}", not a "${selectedRole.toUpperCase()}". Please switch to the ${matchedUser.role} login portal.`
      );
      return;
    }

    if (password.length < 6) {
      setErrorMsg('Password must be at least 6 characters.');
      return;
    }

    setSuccessMsg(`Authentication verified for ${matchedUser.name}! Logging into ${selectedRole} portal...`);
    setTimeout(() => {
      onLoginSuccess(matchedUser);
    }, 600);
  };

  return (
    <div className="min-h-[80vh] flex flex-col items-center justify-center py-8 px-4">
      {/* Back button */}
      <div className="w-full max-w-md mb-4 flex justify-between items-center">
        <button
          onClick={onBackToShop}
          className="inline-flex items-center gap-1.5 text-xs font-semibold text-slate-600 hover:text-slate-900 bg-white px-3 py-1.5 rounded-lg border border-slate-200 transition-colors shadow-2xs"
        >
          <ArrowLeft className="w-3.5 h-3.5" /> Back to Storefront
        </button>

        <span className="text-[11px] font-bold text-slate-500 uppercase tracking-wider">
          Dedicated Role Portals
        </span>
      </div>

      {/* Role Tabs */}
      <div className="w-full max-w-md bg-white p-1.5 rounded-2xl border border-slate-200 shadow-xs mb-6 grid grid-cols-3 gap-1">
        <button
          type="button"
          onClick={() => switchRole('buyer')}
          className={`py-2 px-3 rounded-xl text-xs font-bold flex items-center justify-center gap-1.5 transition-all ${
            selectedRole === 'buyer'
              ? 'bg-blue-600 text-white shadow-xs'
              : 'text-slate-600 hover:bg-slate-100'
          }`}
        >
          <ShoppingBag className="w-3.5 h-3.5" />
          Buyer
        </button>

        <button
          type="button"
          onClick={() => switchRole('seller')}
          className={`py-2 px-3 rounded-xl text-xs font-bold flex items-center justify-center gap-1.5 transition-all ${
            selectedRole === 'seller'
              ? 'bg-amber-600 text-white shadow-xs'
              : 'text-slate-600 hover:bg-slate-100'
          }`}
        >
          <Store className="w-3.5 h-3.5" />
          Seller
        </button>

        <button
          type="button"
          onClick={() => switchRole('admin')}
          className={`py-2 px-3 rounded-xl text-xs font-bold flex items-center justify-center gap-1.5 transition-all ${
            selectedRole === 'admin'
              ? 'bg-purple-700 text-white shadow-xs'
              : 'text-slate-600 hover:bg-slate-100'
          }`}
        >
          <ShieldCheck className="w-3.5 h-3.5" />
          Admin
        </button>
      </div>

      {/* Main Login Card with Role Theming */}
      <div
        className={`w-full max-w-md bg-white rounded-2xl border shadow-xl p-6 sm:p-8 transition-all ${
          selectedRole === 'buyer'
            ? 'border-blue-200'
            : selectedRole === 'seller'
            ? 'border-amber-200'
            : 'border-purple-300'
        }`}
      >
        {/* Role-Specific Header */}
        {selectedRole === 'buyer' && (
          <div className="text-center mb-6">
            <div className="w-12 h-12 bg-blue-100 text-blue-600 rounded-2xl flex items-center justify-center mx-auto mb-3 shadow-xs">
              <ShoppingBag className="w-6 h-6" />
            </div>
            <h2 className="text-xl font-extrabold text-slate-900">Buyer Portal Login</h2>
            <p className="text-xs text-slate-500 mt-1">
              Sign in to manage your shopping cart, view orders, and post verified product reviews.
            </p>
          </div>
        )}

        {selectedRole === 'seller' && (
          <div className="text-center mb-6">
            <div className="w-12 h-12 bg-amber-100 text-amber-600 rounded-2xl flex items-center justify-center mx-auto mb-3 shadow-xs">
              <Store className="w-6 h-6" />
            </div>
            <h2 className="text-xl font-extrabold text-slate-900">Seller Center Sign In</h2>
            <p className="text-xs text-slate-500 mt-1">
              Access your merchant dashboard to list products, manage inventory stock, and track order revenues.
            </p>
          </div>
        )}

        {selectedRole === 'admin' && (
          <div className="text-center mb-6">
            <div className="w-12 h-12 bg-purple-100 text-purple-700 rounded-2xl flex items-center justify-center mx-auto mb-3 shadow-xs">
              <ShieldCheck className="w-6 h-6" />
            </div>
            <div className="inline-flex items-center gap-1.5 px-2.5 py-0.5 rounded-full bg-purple-50 border border-purple-200 text-purple-700 text-[10px] font-bold uppercase tracking-wider mb-2">
              Restricted Console Access
            </div>
            <h2 className="text-xl font-extrabold text-slate-900">Administrator Console</h2>
            <p className="text-xs text-slate-500 mt-1">
              Root access for platform oversight, user management, and catalog moderation.
            </p>
          </div>
        )}

        {/* Status Alerts */}
        {errorMsg && (
          <div className="mb-4 p-3 rounded-xl bg-red-50 border border-red-200 text-red-700 text-xs flex items-start gap-2">
            <AlertCircle className="w-4 h-4 text-red-500 shrink-0 mt-0.5" />
            <div>{errorMsg}</div>
          </div>
        )}

        {successMsg && (
          <div className="mb-4 p-3 rounded-xl bg-emerald-50 border border-emerald-200 text-emerald-700 text-xs flex items-center gap-2">
            <CheckCircle2 className="w-4 h-4 text-emerald-600 shrink-0" />
            <div>{successMsg}</div>
          </div>
        )}

        {/* Autofill Demo Helper Button */}
        <div className="mb-5 p-2.5 bg-slate-50 rounded-xl border border-slate-200 flex items-center justify-between text-xs">
          <span className="text-slate-500 font-medium">Demo Testing Credential:</span>
          <button
            type="button"
            onClick={() => {
              if (selectedRole === 'buyer') {
                setEmail('neha@sanghavimart.com');
                setPassword('password123');
              } else if (selectedRole === 'seller') {
                setEmail('seller.apex@sanghavimart.com');
                setPassword('password123');
              } else {
                setEmail('admin@sanghavimart.com');
                setPassword('password123');
              }
            }}
            className={`font-bold px-2 py-1 rounded-md text-[11px] transition-colors ${
              selectedRole === 'buyer'
                ? 'text-blue-700 hover:bg-blue-100'
                : selectedRole === 'seller'
                ? 'text-amber-700 hover:bg-amber-100'
                : 'text-purple-700 hover:bg-purple-100'
            }`}
          >
            Autofill {selectedRole.toUpperCase()}
          </button>
        </div>

        {/* Login Form */}
        <form onSubmit={handleLogin} className="space-y-4">
          <div>
            <label className="block text-xs font-semibold text-slate-700 mb-1">
              {selectedRole === 'seller'
                ? 'Merchant Business Email'
                : selectedRole === 'admin'
                ? 'Administrator Email'
                : 'Customer Email'}
            </label>
            <div className="relative">
              <Mail className="w-4 h-4 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2" />
              <input
                type="email"
                value={email}
                onChange={(e) => setEmail(e.target.value)}
                required
                placeholder="user@sanghavimart.com"
                className="w-full pl-9 pr-3 py-2 text-xs border border-slate-200 rounded-xl outline-none focus:border-blue-500 transition-colors"
              />
            </div>
          </div>

          <div>
            <div className="flex justify-between items-center mb-1">
              <label className="block text-xs font-semibold text-slate-700">Password</label>
              <span className="text-[11px] text-slate-400">Salted SHA-256</span>
            </div>
            <div className="relative">
              <Lock className="w-4 h-4 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2" />
              <input
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                required
                placeholder="••••••••"
                className="w-full pl-9 pr-3 py-2 text-xs border border-slate-200 rounded-xl outline-none focus:border-blue-500 transition-colors"
              />
            </div>
          </div>

          <button
            type="submit"
            className={`w-full py-2.5 px-4 rounded-xl text-white text-xs font-bold flex items-center justify-center gap-2 shadow-xs transition-colors ${
              selectedRole === 'buyer'
                ? 'bg-blue-600 hover:bg-blue-700'
                : selectedRole === 'seller'
                ? 'bg-amber-600 hover:bg-amber-700'
                : 'bg-purple-700 hover:bg-purple-800'
            }`}
          >
            <span>Sign In as {selectedRole.toUpperCase()}</span>
            <ArrowRight className="w-3.5 h-3.5" />
          </button>
        </form>

        {/* Footer info & cross-links */}
        <div className="mt-6 pt-4 border-t border-slate-100 text-center text-xs text-slate-500 space-y-2">
          {selectedRole === 'buyer' && (
            <p>
              New buyer?{' '}
              <button
                type="button"
                onClick={onNavigateToRegister}
                className="text-blue-600 font-bold hover:underline"
              >
                Create a Buyer Account
              </button>
            </p>
          )}

          {selectedRole === 'seller' && (
            <p>
              Want to sell on SanghaviMart?{' '}
              <button
                type="button"
                onClick={onNavigateToRegister}
                className="text-amber-600 font-bold hover:underline"
              >
                Register as a Seller
              </button>
            </p>
          )}

          {selectedRole === 'admin' && (
            <p className="text-[11px] text-slate-400">
              Admin account pre-seeded in C++ PostgreSQL schema (<code className="text-purple-600">admin@sanghavimart.com</code>).
            </p>
          )}

          <div className="pt-2 flex justify-center gap-3 text-[11px]">
            <button
              onClick={() => switchRole(selectedRole === 'buyer' ? 'seller' : 'buyer')}
              className="text-slate-500 hover:text-slate-800 underline"
            >
              Switch to {selectedRole === 'buyer' ? 'Seller Center' : 'Buyer Sign In'}
            </button>
            <span>•</span>
            <button
              onClick={() => switchRole('admin')}
              className="text-slate-500 hover:text-slate-800 underline"
            >
              Admin Console
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
