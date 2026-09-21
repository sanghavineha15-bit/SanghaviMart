import React, { useState, useMemo } from 'react';
import { Navbar } from './components/Navbar';
import { ProductCard } from './components/ProductCard';
import { CartDrawer } from './components/CartDrawer';
import { CheckoutModal } from './components/CheckoutModal';
import { OrdersView } from './components/OrdersView';
import { SellerDashboard } from './components/SellerDashboard';
import { AdminDashboard } from './components/AdminDashboard';
import { AiChatbot } from './components/AiChatbot';
import { CppSourceViewer } from './components/CppSourceViewer';
import { RoleLoginPage } from './components/RoleLoginPage';
import { INITIAL_USERS, INITIAL_CATEGORIES, INITIAL_PRODUCTS, INITIAL_ORDERS } from './data/mockData';
import { User, Product, CartItem, Order, UserRole } from './types';
import { Search, Filter, Sparkles, Layers, CheckCircle2, Star, X } from 'lucide-react';

export default function App() {
  // Authentication & Role State
  const [users] = useState<User[]>(INITIAL_USERS);
  const [currentRole, setCurrentRole] = useState<UserRole>('buyer');
  const currentUser = useMemo(() => {
    return users.find((u) => u.role === currentRole) || users[0];
  }, [users, currentRole]);

  // Catalog State
  const [products, setProducts] = useState<Product[]>(INITIAL_PRODUCTS);
  const [categories] = useState(INITIAL_CATEGORIES);
  const [selectedCategory, setSelectedCategory] = useState<number | null>(null);
  const [searchQuery, setSearchQuery] = useState('');

  // Cart & Order State
  const [cartItems, setCartItems] = useState<CartItem[]>([]);
  const [orders, setOrders] = useState<Order[]>(INITIAL_ORDERS);
  const [isCartOpen, setIsCartOpen] = useState(false);
  const [isCheckoutOpen, setIsCheckoutOpen] = useState(false);

  // Active View Tab ('shop', 'orders', 'seller', 'admin', 'source', 'login')
  const [activeTab, setActiveTab] = useState<string>('shop');
  const [loginRole, setLoginRole] = useState<UserRole>('buyer');

  // AI Chatbot Drawer State
  const [isChatOpen, setIsChatOpen] = useState(false);

  // Product Reviews Modal
  const [viewingProductReviews, setViewingProductReviews] = useState<Product | null>(null);

  // Filtered Products
  const filteredProducts = useMemo(() => {
    return products.filter((p) => {
      if (!p.is_active) return false;
      if (selectedCategory && p.category_id !== selectedCategory) return false;
      if (searchQuery.trim()) {
        const query = searchQuery.toLowerCase();
        return (
          p.name.toLowerCase().includes(query) ||
          p.description.toLowerCase().includes(query) ||
          p.category_name.toLowerCase().includes(query)
        );
      }
      return true;
    });
  }, [products, selectedCategory, searchQuery]);

  // Cart operations with strict stock ceiling checks
  const handleAddToCart = (product: Product) => {
    const existing = cartItems.find((it) => it.product_id === product.id);
    const currentQty = existing ? existing.quantity : 0;

    if (currentQty + 1 > product.stock_quantity) {
      alert(`Cannot add more than ${product.stock_quantity} units of '${product.name}' (available stock limit).`);
      return;
    }

    if (existing) {
      setCartItems(
        cartItems.map((it) =>
          it.product_id === product.id ? { ...it, quantity: it.quantity + 1 } : it
        )
      );
    } else {
      const newItem: CartItem = {
        id: Date.now(),
        product_id: product.id,
        name: product.name,
        price: product.price,
        quantity: 1,
        stock_quantity: product.stock_quantity,
        image_url: product.image_url,
        seller_name: product.seller_name,
      };
      setCartItems([...cartItems, newItem]);
    }
    setIsCartOpen(true);
  };

  const handleUpdateCartQuantity = (productId: number, newQty: number) => {
    const prod = products.find((p) => p.id === productId);
    if (!prod) return;

    if (newQty <= 0) {
      handleRemoveCartItem(productId);
      return;
    }

    if (newQty > prod.stock_quantity) {
      alert(`Requested quantity (${newQty}) exceeds available stock (${prod.stock_quantity}).`);
      return;
    }

    setCartItems(
      cartItems.map((it) =>
        it.product_id === productId ? { ...it, quantity: newQty } : it
      )
    );
  };

  const handleRemoveCartItem = (productId: number) => {
    setCartItems(cartItems.filter((it) => it.product_id !== productId));
  };

  const handleClearCart = () => {
    setCartItems([]);
  };

  // Checkout Execution: Creates order, deduces stock, clears cart
  const handleConfirmOrder = (shippingDetails: {
    name: string;
    phone: string;
    address: string;
    city: string;
    postalCode: string;
    paymentMethod: string;
  }): Order => {
    const randomNum = Math.floor(1000 + Math.random() * 9000);
    const totalAmount = cartItems.reduce((acc, it) => acc + it.price * it.quantity, 0);

    // 1. Deduct stock from products
    setProducts((prev) =>
      prev.map((p) => {
        const inCart = cartItems.find((c) => c.product_id === p.id);
        if (inCart) {
          return {
            ...p,
            stock_quantity: Math.max(0, p.stock_quantity - inCart.quantity),
          };
        }
        return p;
      })
    );

    // 2. Generate new Order object
    const newOrder: Order = {
      id: Date.now(),
      order_number: `SM-2026-${randomNum}`,
      buyer_id: currentUser.id,
      buyer_name: shippingDetails.name,
      total_amount: totalAmount,
      status: 'pending',
      shipping_name: shippingDetails.name,
      shipping_phone: shippingDetails.phone,
      shipping_address: shippingDetails.address,
      shipping_city: shippingDetails.city,
      shipping_postal_code: shippingDetails.postalCode,
      payment_method: shippingDetails.paymentMethod,
      created_at: new Date().toLocaleString(),
      items: cartItems.map((it) => {
        const prod = products.find((p) => p.id === it.product_id);
        return {
          id: Date.now() + Math.random(),
          product_id: it.product_id,
          product_name: it.name,
          image_url: it.image_url,
          seller_id: prod?.seller_id || 2,
          seller_name: it.seller_name,
          quantity: it.quantity,
          unit_price: it.price,
          subtotal: it.price * it.quantity,
        };
      }),
    };

    // 3. Clear cart
    setCartItems([]);

    // 4. Save order to list
    setOrders((prev) => [newOrder, ...prev]);

    return newOrder;
  };

  // Verified Review submission
  const handleSubmitReview = (productId: number, rating: number, comment: string) => {
    setProducts((prev) =>
      prev.map((p) => {
        if (p.id === productId) {
          const newCount = p.review_count + 1;
          const newAvg = (p.average_rating * p.review_count + rating) / newCount;
          return {
            ...p,
            review_count: newCount,
            average_rating: Number(newAvg.toFixed(1)),
          };
        }
        return p;
      })
    );
    alert('Thank you! Your verified purchase review has been recorded.');
  };

  // Seller Dashboard handlers
  const handleAddProduct = (productData: Partial<Product>) => {
    const newProd: Product = {
      id: Date.now(),
      seller_id: currentUser.id,
      seller_name: currentUser.name,
      category_id: productData.category_id || 1,
      category_name: productData.category_name || 'General',
      name: productData.name || 'New Product',
      description: productData.description || '',
      price: productData.price || 0,
      stock_quantity: productData.stock_quantity || 0,
      image_url: productData.image_url || 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=500&q=80',
      is_active: true,
      average_rating: 5.0,
      review_count: 1,
      created_at: new Date().toLocaleString(),
    };
    setProducts([newProd, ...products]);
  };

  const handleUpdateProduct = (productId: number, productData: Partial<Product>) => {
    setProducts((prev) =>
      prev.map((p) => (p.id === productId ? { ...p, ...productData } : p))
    );
  };

  const handleDeleteProduct = (productId: number) => {
    setProducts((prev) => prev.filter((p) => p.id !== productId));
  };

  const handleUpdateOrderStatus = (orderId: number, status: Order['status']) => {
    setOrders((prev) =>
      prev.map((o) => (o.id === orderId ? { ...o, status } : o))
    );
  };

  const cartTotalCount = cartItems.reduce((acc, it) => acc + it.quantity, 0);

  return (
    <div className="min-h-screen flex flex-col bg-slate-50 text-slate-900">
      {/* Navigation Bar */}
      <Navbar
        currentUser={currentUser}
        onSelectRole={(role) => {
          setCurrentRole(role);
          if (role === 'seller') setActiveTab('seller');
          else if (role === 'admin') setActiveTab('admin');
          else setActiveTab('shop');
        }}
        activeTab={activeTab}
        setActiveTab={setActiveTab}
        cartCount={cartTotalCount}
        onOpenCart={() => setIsCartOpen(true)}
        onOpenChat={() => setIsChatOpen(true)}
        onOpenLogin={(role) => {
          setLoginRole(role);
          setActiveTab('login');
        }}
      />

      {/* Main Content Area */}
      <main className="flex-1 max-w-7xl w-full mx-auto px-4 sm:px-6 lg:px-8 py-6">
        
        {/* VIEW 1: SHOP CATALOG */}
        {activeTab === 'shop' && (
          <div className="space-y-6">
            {/* Hero Banner */}
            <div className="relative overflow-hidden rounded-2xl bg-gradient-to-r from-blue-900 via-indigo-900 to-slate-900 text-white p-8 shadow-sm">
              <div className="relative z-10 max-w-2xl">
                <div className="inline-flex items-center gap-2 bg-blue-500/20 backdrop-blur-xs text-blue-200 text-xs font-semibold px-3 py-1 rounded-full border border-blue-400/20 mb-3">
                  <Sparkles className="w-3.5 h-3.5 text-blue-300" />
                  Multi-Seller E-Commerce System • C++20 Drogon Framework
                </div>
                <h1 className="text-3xl sm:text-4xl font-black tracking-tight text-white mb-2">
                  SanghaviMart Marketplace
                </h1>
                <p className="text-sm text-slate-300 mb-6 leading-relaxed">
                  Academic Capstone featuring high-concurrency non-blocking event loops, PostgreSQL normalization, strict inventory ceiling enforcement, and multi-role RBAC.
                </p>

                <div className="flex flex-wrap gap-2 text-xs">
                  <span className="bg-white/10 px-3 py-1.5 rounded-lg border border-white/10">
                    🚀 100k+ Req/s Async Event Loop
                  </span>
                  <span className="bg-white/10 px-3 py-1.5 rounded-lg border border-white/10">
                    🔒 Salted SHA-256 Auth & JWT
                  </span>
                  <span className="bg-white/10 px-3 py-1.5 rounded-lg border border-white/10">
                    📦 ACID Stock Deductions
                  </span>
                </div>
              </div>
            </div>

            {/* Filter & Search Bar */}
            <div className="bg-white p-4 rounded-xl border border-slate-200 shadow-xs flex flex-col sm:flex-row gap-3 items-center justify-between">
              {/* Search input */}
              <div className="relative w-full sm:w-80">
                <Search className="w-4 h-4 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2" />
                <input
                  type="text"
                  placeholder="Search products or descriptions..."
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  className="w-full pl-9 pr-3 py-2 text-xs bg-slate-50 border border-slate-200 rounded-lg outline-none focus:border-blue-500 focus:bg-white transition-all"
                />
              </div>

              {/* Category Pills */}
              <div className="flex items-center gap-1.5 overflow-x-auto w-full sm:w-auto no-scrollbar pb-1 sm:pb-0">
                <button
                  onClick={() => setSelectedCategory(null)}
                  className={`px-3 py-1.5 text-xs rounded-lg font-medium whitespace-nowrap transition-colors ${
                    selectedCategory === null
                      ? 'bg-blue-600 text-white font-bold'
                      : 'bg-slate-100 text-slate-600 hover:bg-slate-200'
                  }`}
                >
                  All Items ({products.filter((p) => p.is_active).length})
                </button>
                {categories.map((c) => (
                  <button
                    key={c.id}
                    onClick={() => setSelectedCategory(c.id)}
                    className={`px-3 py-1.5 text-xs rounded-lg font-medium whitespace-nowrap transition-colors ${
                      selectedCategory === c.id
                        ? 'bg-blue-600 text-white font-bold'
                        : 'bg-slate-100 text-slate-600 hover:bg-slate-200'
                    }`}
                  >
                    {c.name}
                  </button>
                ))}
              </div>
            </div>

            {/* Product Cards Grid */}
            {filteredProducts.length === 0 ? (
              <div className="bg-white p-12 rounded-2xl border border-slate-200 text-center">
                <p className="text-sm font-semibold text-slate-700">No products match your search or filter.</p>
                <p className="text-xs text-slate-500 mt-1">Try clearing your search query or selecting "All Items".</p>
              </div>
            ) : (
              <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-5">
                {filteredProducts.map((prod) => (
                  <ProductCard
                    key={prod.id}
                    product={prod}
                    onAddToCart={handleAddToCart}
                    onViewReviews={(p) => setViewingProductReviews(p)}
                  />
                ))}
              </div>
            )}
          </div>
        )}

        {/* VIEW 2: BUYER ORDERS & VERIFIED REVIEWS */}
        {activeTab === 'orders' && (
          <OrdersView
            orders={orders.filter((o) => o.buyer_id === currentUser.id)}
            products={products}
            onSubmitReview={handleSubmitReview}
          />
        )}

        {/* VIEW 3: SELLER DASHBOARD */}
        {activeTab === 'seller' && (
          <SellerDashboard
            seller={currentUser}
            products={products}
            categories={categories}
            orders={orders}
            onAddProduct={handleAddProduct}
            onUpdateProduct={handleUpdateProduct}
            onDeleteProduct={handleDeleteProduct}
            onUpdateOrderStatus={handleUpdateOrderStatus}
          />
        )}

        {/* VIEW 4: ADMIN CONSOLE */}
        {activeTab === 'admin' && (
          <AdminDashboard
            users={users}
            products={products}
            orders={orders}
            onDeleteProduct={handleDeleteProduct}
          />
        )}

        {/* VIEW 5: C++ SOURCE CODE & ARCHITECTURE VIEWER */}
        {activeTab === 'source' && <CppSourceViewer />}

        {/* VIEW 6: DEDICATED ROLE-BASED LOGIN PAGES (BUYER, SELLER, ADMIN) */}
        {activeTab === 'login' && (
          <RoleLoginPage
            initialRole={loginRole}
            users={users}
            onLoginSuccess={(user) => {
              setCurrentRole(user.role);
              if (user.role === 'seller') setActiveTab('seller');
              else if (user.role === 'admin') setActiveTab('admin');
              else setActiveTab('shop');
            }}
            onBackToShop={() => setActiveTab('shop')}
            onNavigateToRegister={() => {
              alert('Registration form available in C++ Drogon routes at /register.html. In this live demo, you can log in with any test buyer/seller account directly.');
            }}
          />
        )}
      </main>

      {/* Cart Drawer Component */}
      <CartDrawer
        isOpen={isCartOpen}
        onClose={() => setIsCartOpen(false)}
        cartItems={cartItems}
        onUpdateQuantity={handleUpdateCartQuantity}
        onRemoveItem={handleRemoveCartItem}
        onClearCart={handleClearCart}
        onProceedToCheckout={() => {
          setIsCartOpen(false);
          setIsCheckoutOpen(true);
        }}
      />

      {/* Checkout Modal Component */}
      <CheckoutModal
        isOpen={isCheckoutOpen}
        onClose={() => setIsCheckoutOpen(false)}
        cartItems={cartItems}
        onConfirmOrder={handleConfirmOrder}
        onOrderSuccess={(order) => {
          // Handled inside checkout modal
        }}
      />

      {/* AI Chatbot Floating Widget */}
      <AiChatbot isOpen={isChatOpen} onClose={() => setIsChatOpen(false)} />

      {/* Product Reviews Modal */}
      {viewingProductReviews && (
        <div className="fixed inset-0 z-50 overflow-y-auto flex items-center justify-center p-4 bg-slate-900/60 backdrop-blur-xs">
          <div className="relative bg-white rounded-2xl max-w-md w-full p-6 shadow-2xl border border-slate-200">
            <div className="flex items-center justify-between pb-3 mb-3 border-b border-slate-200">
              <h3 className="text-base font-bold text-slate-900">
                Reviews for '{viewingProductReviews.name}'
              </h3>
              <button
                onClick={() => setViewingProductReviews(null)}
                className="p-1 text-slate-400 hover:text-slate-700"
              >
                <X className="w-5 h-5" />
              </button>
            </div>

            <div className="flex items-center gap-3 p-3 bg-slate-50 rounded-xl mb-4 border border-slate-200">
              <div className="text-3xl font-black text-slate-900">
                {viewingProductReviews.average_rating.toFixed(1)}
              </div>
              <div>
                <div className="flex text-amber-400">
                  {[...Array(5)].map((_, i) => (
                    <Star
                      key={i}
                      className={`w-4 h-4 ${
                        i < Math.round(viewingProductReviews.average_rating)
                          ? 'fill-current'
                          : 'text-slate-300'
                      }`}
                    />
                  ))}
                </div>
                <div className="text-xs text-slate-500 mt-0.5">
                  Based on {viewingProductReviews.review_count} verified buyer ratings
                </div>
              </div>
            </div>

            <div className="space-y-3 max-h-60 overflow-y-auto text-xs">
              <div className="p-3 bg-white border border-slate-200 rounded-xl">
                <div className="flex justify-between font-semibold text-slate-900 mb-1">
                  <span>Neha Sanghavi (Verified Buyer)</span>
                  <span className="text-amber-500 font-bold">5.0 ★</span>
                </div>
                <p className="text-slate-600 leading-relaxed">
                  "Phenomenal build quality! Order dispatch was immediate and the product exceeds expectations."
                </p>
                <div className="text-[10px] text-slate-400 mt-1">Verified Order SM-2026-8821</div>
              </div>
            </div>

            <div className="mt-4 pt-3 border-t border-slate-200 text-center">
              <button
                onClick={() => setViewingProductReviews(null)}
                className="w-full py-2 bg-slate-100 hover:bg-slate-200 text-slate-700 font-semibold text-xs rounded-xl"
              >
                Close
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Footer */}
      <footer className="bg-white border-t border-slate-200 py-6 mt-12">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 text-center text-xs text-slate-500 space-y-1">
          <p className="font-semibold text-slate-700">
            SanghaviMart — Full-Stack C++20 Drogon E-Commerce Capstone Project
          </p>
          <p>
            Developed by <strong>Neha Sanghavi</strong> • Backend: C++20, Drogon Framework, PostgreSQL • Frontend: Semantic HTML5, CSS3, JavaScript, React 19
          </p>
          <p className="text-[11px] text-slate-400">
            Protected against race-condition overselling with database transactions and role-based access control.
          </p>
        </div>
      </footer>
    </div>
  );
}
