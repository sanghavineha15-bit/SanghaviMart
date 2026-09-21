// SanghaviMart Client API SDK
const API_BASE = '/api';

const api = {
  getToken() {
    return localStorage.getItem('sm_token') || '';
  },
  setToken(token) {
    localStorage.setItem('sm_token', token);
  },
  getUser() {
    const u = localStorage.getItem('sm_user');
    return u ? JSON.parse(u) : null;
  },
  setUser(user) {
    localStorage.setItem('sm_user', JSON.stringify(user));
  },
  clearAuth() {
    localStorage.removeItem('sm_token');
    localStorage.removeItem('sm_user');
  },

  async request(endpoint, options = {}) {
    const headers = {
      'Content-Type': 'application/json',
      ...(options.headers || {}),
    };
    const token = this.getToken();
    if (token) {
      headers['Authorization'] = `Bearer ${token}`;
    }

    try {
      const resp = await fetch(`${API_BASE}${endpoint}`, {
        ...options,
        headers,
      });
      const data = await resp.json();
      if (!resp.ok) {
        throw new Error(data.error || `HTTP Error ${resp.status}`);
      }
      return data;
    } catch (err) {
      console.error(`API Error [${endpoint}]:`, err);
      throw err;
    }
  },

  // Auth
  register(payload) {
    return this.request('/register', { method: 'POST', body: JSON.stringify(payload) });
  },
  login(payload) {
    return this.request('/login', { method: 'POST', body: JSON.stringify(payload) });
  },
  logout() {
    this.clearAuth();
    return this.request('/logout', { method: 'POST' });
  },
  getMe() {
    return this.request('/auth/me');
  },

  // Products
  getProducts(params = {}) {
    const query = new URLSearchParams(params).toString();
    return this.request(`/products${query ? '?' + query : ''}`);
  },
  getProductById(id) {
    return this.request(`/products/${id}`);
  },
  addProduct(product) {
    return this.request('/products', { method: 'POST', body: JSON.stringify(product) });
  },
  updateProduct(id, product) {
    return this.request(`/products/${id}`, { method: 'PUT', body: JSON.stringify(product) });
  },
  deleteProduct(id) {
    return this.request(`/products/${id}`, { method: 'DELETE' });
  },
  getCategories() {
    return this.request('/categories');
  },

  // Cart
  getCart() {
    return this.request('/cart');
  },
  addToCart(productId, quantity = 1) {
    return this.request('/cart', { method: 'POST', body: JSON.stringify({ product_id: productId, quantity }) });
  },
  updateCartItem(itemId, quantity) {
    return this.request(`/cart/${itemId}`, { method: 'PUT', body: JSON.stringify({ quantity }) });
  },
  removeCartItem(itemId) {
    return this.request(`/cart/${itemId}`, { method: 'DELETE' });
  },
  clearCart() {
    return this.request('/cart', { method: 'DELETE' });
  },

  // Orders
  createOrder(shippingData) {
    return this.request('/orders', { method: 'POST', body: JSON.stringify(shippingData) });
  },
  getOrders() {
    return this.request('/orders');
  },
  getOrderById(id) {
    return this.request(`/orders/${id}`);
  },
  updateOrderStatus(orderId, status) {
    return this.request(`/orders/${orderId}/status`, { method: 'PUT', body: JSON.stringify({ status }) });
  },

  // Reviews
  addReview(reviewData) {
    return this.request('/reviews', { method: 'POST', body: JSON.stringify(reviewData) });
  },
  getProductReviews(productId) {
    return this.request(`/products/${productId}/reviews`);
  },

  // Admin
  getAdminUsers(role = '') {
    return this.request(`/admin/users${role ? '?role=' + role : ''}`);
  },
  getAdminProducts() {
    return this.request('/admin/products');
  },
  getAdminOrders() {
    return this.request('/admin/orders');
  },
  getAdminStats() {
    return this.request('/admin/stats');
  },
  adminDeleteProduct(id) {
    return this.request(`/admin/products/${id}`, { method: 'DELETE' });
  },

  // AI Chatbot
  sendChatMessage(message) {
    return this.request('/chat', { method: 'POST', body: JSON.stringify({ message }) });
  },
};
