import React, { useState } from 'react';
import { ShoppingCart, Bot, Code, ShieldCheck, Store, Package, User as UserIcon, LogIn, ChevronDown } from 'lucide-react';
import { User, UserRole } from '../types';

interface NavbarProps {
  currentUser: User;
  onSelectRole: (role: UserRole) => void;
  activeTab: string;
  setActiveTab: (tab: string) => void;
  cartCount: number;
  onOpenCart: () => void;
  onOpenChat: () => void;
  onOpenLogin: (role: UserRole) => void;
}

export const Navbar: React.FC<NavbarProps> = ({
  currentUser,
  onSelectRole,
  activeTab,
  setActiveTab,
  cartCount,
  onOpenCart,
  onOpenChat,
  onOpenLogin,
}) => {
  const [isLoginMenuOpen, setIsLoginMenuOpen] = useState(false);
  return (
    <header className="sticky top-0 z-40 bg-white border-b border-slate-200 shadow-xs">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16 gap-4">
          
          {/* Brand */}
          <div className="flex items-center gap-3 cursor-pointer" onClick={() => setActiveTab('shop')}>
            <div className="w-10 h-10 rounded-xl bg-blue-600 text-white flex items-center justify-center font-bold text-xl shadow-sm">
              SM
            </div>
            <div>
              <div className="flex items-center gap-2">
                <span className="font-extrabold text-lg tracking-tight text-slate-900">SanghaviMart</span>
                <span className="text-[10px] font-bold uppercase tracking-wider bg-blue-50 text-blue-700 px-2 py-0.5 rounded-full border border-blue-200">
                  C++20 Drogon
                </span>
              </div>
              <p className="text-xs text-slate-500 hidden sm:block">Multi-Seller Academic Capstone</p>
            </div>
          </div>

          {/* Navigation Items */}
          <nav className="hidden md:flex items-center gap-1 text-sm font-medium">
            <button
              onClick={() => setActiveTab('shop')}
              className={`px-3 py-2 rounded-lg transition-colors ${
                activeTab === 'shop' ? 'bg-blue-50 text-blue-600 font-semibold' : 'text-slate-600 hover:text-slate-900'
              }`}
            >
              Shop Catalog
            </button>

            {currentUser.role === 'buyer' && (
              <button
                onClick={() => setActiveTab('orders')}
                className={`px-3 py-2 rounded-lg transition-colors flex items-center gap-1.5 ${
                  activeTab === 'orders' ? 'bg-blue-50 text-blue-600 font-semibold' : 'text-slate-600 hover:text-slate-900'
                }`}
              >
                <Package className="w-4 h-4" />
                My Orders
              </button>
            )}

            {currentUser.role === 'seller' && (
              <button
                onClick={() => setActiveTab('seller')}
                className={`px-3 py-2 rounded-lg transition-colors flex items-center gap-1.5 ${
                  activeTab === 'seller' ? 'bg-amber-50 text-amber-700 font-semibold' : 'text-slate-600 hover:text-slate-900'
                }`}
              >
                <Store className="w-4 h-4" />
                Seller Dashboard
              </button>
            )}

            {currentUser.role === 'admin' && (
              <button
                onClick={() => setActiveTab('admin')}
                className={`px-3 py-2 rounded-lg transition-colors flex items-center gap-1.5 ${
                  activeTab === 'admin' ? 'bg-purple-50 text-purple-700 font-semibold' : 'text-slate-600 hover:text-slate-900'
                }`}
              >
                <ShieldCheck className="w-4 h-4" />
                Admin Console
              </button>
            )}

            <button
              onClick={() => setActiveTab('source')}
              className={`px-3 py-2 rounded-lg transition-colors flex items-center gap-1.5 ${
                activeTab === 'source' ? 'bg-emerald-50 text-emerald-700 font-semibold' : 'text-slate-600 hover:text-slate-900'
              }`}
            >
              <Code className="w-4 h-4" />
              C++ Source Code & Architecture
            </button>
          </nav>

          {/* Right Controls: Role switcher, Cart, Chat */}
          <div className="flex items-center gap-2 sm:gap-3">
            
            {/* Quick Role Switcher Pill */}
            <div className="flex items-center bg-slate-100 p-1 rounded-lg border border-slate-200">
              <span className="text-[11px] font-semibold text-slate-500 px-1.5 hidden lg:inline">Role:</span>
              <button
                onClick={() => onSelectRole('buyer')}
                className={`px-2 py-1 text-xs rounded-md font-medium transition-all ${
                  currentUser.role === 'buyer'
                    ? 'bg-white text-blue-700 shadow-xs font-semibold'
                    : 'text-slate-600 hover:text-slate-900'
                }`}
              >
                Buyer
              </button>
              <button
                onClick={() => onSelectRole('seller')}
                className={`px-2 py-1 text-xs rounded-md font-medium transition-all ${
                  currentUser.role === 'seller'
                    ? 'bg-white text-amber-700 shadow-xs font-semibold'
                    : 'text-slate-600 hover:text-slate-900'
                }`}
              >
                Seller
              </button>
              <button
                onClick={() => onSelectRole('admin')}
                className={`px-2 py-1 text-xs rounded-md font-medium transition-all ${
                  currentUser.role === 'admin'
                    ? 'bg-white text-purple-700 shadow-xs font-semibold'
                    : 'text-slate-600 hover:text-slate-900'
                }`}
              >
                Admin
              </button>
            </div>

            {/* Dedicated Role Login Menu */}
            <div className="relative">
              <button
                onClick={() => setIsLoginMenuOpen(!isLoginMenuOpen)}
                className="px-2.5 py-1.5 text-xs font-semibold text-slate-700 hover:text-blue-700 hover:bg-slate-100 rounded-lg border border-slate-200 transition-colors flex items-center gap-1.5"
                title="Login Portals"
              >
                <LogIn className="w-3.5 h-3.5 text-blue-600" />
                <span className="hidden sm:inline">Login Portals</span>
                <ChevronDown className="w-3 h-3 text-slate-400" />
              </button>

              {isLoginMenuOpen && (
                <>
                  <div
                    className="fixed inset-0 z-40"
                    onClick={() => setIsLoginMenuOpen(false)}
                  />
                  <div className="absolute right-0 mt-2 w-56 bg-white rounded-xl shadow-xl border border-slate-200 py-1.5 z-50 text-xs animate-in fade-in zoom-in-95 duration-150">
                    <div className="px-3 py-1.5 border-b border-slate-100 font-bold text-[10px] text-slate-400 uppercase tracking-wider">
                      Specific Login Portals
                    </div>
                    
                    <button
                      onClick={() => {
                        onOpenLogin('buyer');
                        setIsLoginMenuOpen(false);
                      }}
                      className="w-full px-3 py-2 text-left hover:bg-blue-50 flex items-center gap-2.5 text-slate-700 hover:text-blue-700 transition-colors"
                    >
                      <div className="w-6 h-6 rounded-md bg-blue-100 text-blue-600 flex items-center justify-center text-xs">
                        🛍️
                      </div>
                      <div>
                        <div className="font-bold text-slate-900">Buyer Login</div>
                        <div className="text-[10px] text-slate-400">Cart, orders & reviews</div>
                      </div>
                    </button>

                    <button
                      onClick={() => {
                        onOpenLogin('seller');
                        setIsLoginMenuOpen(false);
                      }}
                      className="w-full px-3 py-2 text-left hover:bg-amber-50 flex items-center gap-2.5 text-slate-700 hover:text-amber-700 transition-colors"
                    >
                      <div className="w-6 h-6 rounded-md bg-amber-100 text-amber-700 flex items-center justify-center text-xs">
                        🏬
                      </div>
                      <div>
                        <div className="font-bold text-slate-900">Seller Center</div>
                        <div className="text-[10px] text-slate-400">Inventory & store metrics</div>
                      </div>
                    </button>

                    <button
                      onClick={() => {
                        onOpenLogin('admin');
                        setIsLoginMenuOpen(false);
                      }}
                      className="w-full px-3 py-2 text-left hover:bg-purple-50 flex items-center gap-2.5 text-slate-700 hover:text-purple-700 transition-colors"
                    >
                      <div className="w-6 h-6 rounded-md bg-purple-100 text-purple-700 flex items-center justify-center text-xs">
                        🛡️
                      </div>
                      <div>
                        <div className="font-bold text-slate-900">Admin Console</div>
                        <div className="text-[10px] text-slate-400">Root moderation access</div>
                      </div>
                    </button>
                  </div>
                </>
              )}
            </div>

            {/* AI Assistant Button */}
            <button
              onClick={onOpenChat}
              className="p-2 text-slate-700 hover:text-blue-600 hover:bg-blue-50 rounded-lg border border-slate-200 transition-colors flex items-center gap-1.5 text-xs font-medium"
              title="Open AI Shopping Assistant"
            >
              <Bot className="w-4 h-4 text-blue-600" />
              <span className="hidden sm:inline">AI Help</span>
            </button>

            {/* Cart Button */}
            <button
              onClick={onOpenCart}
              className="relative p-2 text-slate-700 hover:text-blue-600 hover:bg-blue-50 rounded-lg border border-slate-200 transition-colors"
              title="View Shopping Cart"
            >
              <ShoppingCart className="w-5 h-5" />
              {cartCount > 0 && (
                <span className="absolute -top-1.5 -right-1.5 bg-blue-600 text-white text-[11px] font-bold w-5 h-5 rounded-full flex items-center justify-center shadow-xs">
                  {cartCount}
                </span>
              )}
            </button>
          </div>
        </div>
      </div>
    </header>
  );
};
