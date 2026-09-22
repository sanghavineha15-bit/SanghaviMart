import express from 'express';
import path from 'path';
import { createServer as createViteServer } from 'vite';
import { GoogleGenAI } from '@google/genai';
import dotenv from 'dotenv';

dotenv.config();

const app = express();
const PORT = 3000;

app.use(express.json());

// Initialize Google Gemini Client lazily with User-Agent header
let aiClient: GoogleGenAI | null = null;
function getGeminiClient(): GoogleGenAI | null {
  const apiKey = process.env.GEMINI_API_KEY;
  if (!apiKey || apiKey === 'MY_GEMINI_API_KEY') {
    return null;
  }
  if (!aiClient) {
    aiClient = new GoogleGenAI({
      apiKey,
      httpOptions: {
        headers: {
          'User-Agent': 'aistudio-build',
        },
      },
    });
  }
  return aiClient;
}

// Health check endpoint
app.get('/api/health', (req, res) => {
  res.json({
    status: 'ok',
    project: 'SanghaviMart C++ Drogon Capstone',
    timestamp: new Date().toISOString(),
  });
});

// AI Chatbot endpoint for SanghaviMart
app.post('/api/chat', async (req, res) => {
  try {
    const { message, history = [], context } = req.body;
    if (!message || typeof message !== 'string') {
      res.status(400).json({ error: 'Message is required' });
      return;
    }

    const ai = getGeminiClient();

    const systemInstruction = `You are SanghaviBot, the friendly, intelligent AI Shopping Assistant for SanghaviMart — a multi-seller C++ Drogon e-commerce platform.
Your goals:
1. Help buyers find products, check stock, evaluate prices, and provide shopping guidance.
2. Explain order placement, checkout flow (Cart -> Shipping Address -> Order Confirmation -> Order ID), and order tracking statuses (Pending, Processing, Shipped, Delivered).
3. Guide sellers on how to register, list products, update inventory, and inspect incoming purchase orders in their Seller Dashboard.
4. Explain Admin capabilities (user verification, catalog moderation, platform metrics).
5. Explain technical C++ Drogon architecture details if the user asks about the backend, PostgreSQL tables, REST APIs, or how the project was built.
Current Store Context:
- Available Categories: Electronics, Fashion, Home & Living, Books, Accessories
- Current active products:
  * "NoisePulse Wireless ANC Headphones" ($79.99, Stock: 15)
  * "Apple MacBook Air M2 13-inch" ($999.00, Stock: 5)
  * "Ergonomic Mesh High-Back Chair" ($189.50, Stock: 8)
  * "Nordic Minimalist Ceramic Mug Set" ($28.00, Stock: 24)
  * "Clean Code: Handbook of Agile Software Craftsmanship" ($42.00, Stock: 12)
  * "Sanghavi Classic Chronograph Watch" ($149.00, Stock: 7)
Keep answers concise, helpful, and polite. Provide markdown formatting when appropriate.`;

    if (ai) {
      // Build conversation context
      const promptText = `${systemInstruction}\n\nUser Question: ${message}`;
      const response = await ai.models.generateContent({
        model: 'gemini-3.8-flash',
        contents: promptText,
      });

      const reply = response.text || "Hello! I am SanghaviBot. How may I assist your shopping experience on SanghaviMart today?";
      res.json({ reply, source: 'gemini-ai' });
      return;
    }

    // High quality rule-based intelligent fallback if API key not supplied
    const query = message.toLowerCase();
    let fallbackReply = "Welcome to SanghaviMart! I can guide you through our product catalog, order flow, seller dashboard, or the C++ Drogon architectural details.";

    if (query.includes('hello') || query.includes('hi') || query.includes('hey')) {
      fallbackReply = "Hello! Welcome to **SanghaviMart**. I am your AI Shopping and System Guide. Are you shopping as a Buyer, managing products as a Seller, or reviewing the C++ Drogon capstone code?";
    } else if (query.includes('cart') || query.includes('buy') || query.includes('checkout') || query.includes('order')) {
      fallbackReply = "**How ordering works on SanghaviMart:**\n1. Browse products and click **Add to Cart** (our C++ backend checks available stock in PostgreSQL).\n2. Open your Cart to adjust quantities.\n3. Proceed to Checkout and enter your shipping details.\n4. Confirm order: stock is immediately decremented and an Order ID is generated!\n5. Track progress under **Order History**.";
    } else if (query.includes('seller') || query.includes('add product')) {
      fallbackReply = "**Seller Guide:**\n- Register or switch to a **Seller** account.\n- Navigate to **Seller Dashboard**.\n- You can add products with custom prices, stock, image URLs, and descriptions.\n- You can edit stock, modify descriptions, or inspect customer orders for your items in real time.";
    } else if (query.includes('admin')) {
      fallbackReply = "**Admin Capabilities:**\n- Protected dashboard for administrators.\n- View all users (Buyers & Sellers), total products, and orders.\n- Remove inappropriate items or moderate platform accounts.";
    } else if (query.includes('laptop') || query.includes('macbook') || query.includes('headphone') || query.includes('product') || query.includes('recommend')) {
      fallbackReply = "**Featured Recommendations on SanghaviMart:**\n- 🎧 **NoisePulse Wireless ANC Headphones** ($79.99) — 15 in stock\n- 💻 **Apple MacBook Air M2** ($999.00) — High performance laptop\n- 🪑 **Ergonomic Mesh Chair** ($189.50) — Ideal for desk work\n- 📚 **Clean Code** ($42.00) — Essential for C++ and software developers!";
    } else if (query.includes('c++') || query.includes('drogon') || query.includes('backend') || query.includes('database') || query.includes('postgres')) {
      fallbackReply = "**Technical Architecture:**\n- **Backend**: C++20 with Drogon framework (non-blocking async I/O based on libtrantor)\n- **Database**: PostgreSQL with connection pooling\n- **Security**: Argon2/Bcrypt password hashing & JWT authentication\n- **API**: RESTful JSON endpoints for Auth, Products, Cart, Orders, Reviews, and Admin\n- You can inspect and export the full C++ source files in the **C++ Codebase & Docs** tab!";
    }

    res.json({ reply: fallbackReply, source: 'system-assistant' });
  } catch (error: any) {
    console.error('Chat error:', error);
    res.status(500).json({
      error: 'Failed to process chat message',
      details: error?.message || 'Unknown error',
    });
  }
});

// Vite middleware configuration
async function startServer() {
  if (process.env.NODE_ENV !== 'production') {
    const vite = await createViteServer({
      server: { middlewareMode: true },
      appType: 'spa',
    });
    app.use(vite.middlewares);
  } else {
    const distPath = path.join(process.cwd(), 'dist');
    app.use(express.static(distPath));
    app.get('*', (req, res) => {
      res.sendFile(path.join(distPath, 'index.html'));
    });
  }

  app.listen(PORT, '0.0.0.0', () => {
    console.log(`SanghaviMart Server running on http://localhost:${PORT}`);
  });
}

startServer();
