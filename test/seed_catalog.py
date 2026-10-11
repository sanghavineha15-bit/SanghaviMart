"""SanghaviMart catalog seeder: 300+ products, 11 categories, every image a
verified-live Unsplash photo whose content matches the product name.
Idempotent: upserts on (seller_id, name). All rows belong to the demo seller,
so they stay editable/deletable from the Seller Dashboard.
Usage: python test/seed_catalog.py   (needs sanghavimart_db on :5434)"""
import sys
import psycopg

DSN = "host=127.0.0.1 port=5434 dbname=sanghavimart user=sanghavi password=sanghavi_pw"
IMG = "https://images.unsplash.com/photo-%s?auto=format&fit=crop&w=600&q=80"

# (photo_id, category, base_name, [(suffix, price_paise, stock, description), ...])
CAT = [
 # ---------------- electronics (12 photos) ----------------
 ("1505740420928-5e560c06d30e", "electronics", "Wireless Headphones", [
   (" Pro", 299900, 40, "Over-ear Bluetooth headphones, 40h battery, mic."),
   (" Studio", 499900, 25, "Studio-tuned drivers, memory-foam cushions."),
   (" Travel Foldable", 199900, 60, "Fold-flat design, carry case included."),
   (" Bass Boost", 249900, 50, "Extra-bass mode for music and movies.")]),
 ("1590658268037-6bf12165a8df", "electronics", "True-Wireless Earbuds", [
   ("", 199900, 80, "In-ear earbuds with charging case, touch controls."),
   (" Pro ANC", 349900, 45, "Active noise cancellation, wireless charging."),
   (" Sport Fit", 229900, 70, "Sweat-proof wingtips for workouts."),
   (" Mini", 149900, 90, "Compact buds, 24h total playtime.")]),
 ("1608043152269-423dbba4e7e1", "electronics", "Bluetooth Speaker", [
   ("", 249900, 55, "Room-filling 20W speaker, 12h playtime."),
   (" Waterproof", 329900, 40, "IPX7 waterproof for pool and rain."),
   (" Party 40W", 499900, 30, "Dual drivers with bass radiators."),
   (" Mini", 149900, 85, "Pocket speaker, 10h battery.")]),
 ("1546868871-7041f2a55e12", "electronics", "Smartwatch", [
   ("", 499900, 50, "1.85-inch display, heart + SpO2 tracking."),
   (" Pro AMOLED", 799900, 30, "AMOLED display, BT calling, 14-day battery."),
   (" Sport", 399900, 60, "100+ sport modes, 5ATM water resistant."),
   (" Classic Leather", 649900, 25, "Leather strap edition, metal dial.")]),
 ("1511707171634-5f897ff02aa9", "electronics", "Smartphone", [
   (" 128GB", 1499900, 35, "6.5-inch 120Hz display, 50MP camera."),
   (" 256GB", 1799900, 25, "Extra storage, same 5000mAh battery."),
   (" 5G Pro", 2499900, 20, "Flagship chipset, OIS camera."),
   (" Lite", 999900, 45, "Everyday 5G phone, 3-day battery.")]),
 ("1496181133206-80ce9b88a853", "electronics", "Laptop", [
   (" 15.6-inch i5", 5499900, 15, "16GB RAM, 512GB SSD, backlit keyboard."),
   (" 14-inch Ultrabook", 6999900, 12, "1.2kg metal body, 18h battery."),
   (" Gaming RTX", 8999900, 8, "RTX graphics, 144Hz display."),
   (" Student 8GB", 3899900, 20, "Lightweight daily driver for classes.")]),
 ("1502920917128-1aa500764cbd", "electronics", "Digital Camera", [
   ("", 3299900, 10, "24MP mirrorless with kit lens."),
   (" Vlogging Kit", 4499900, 8, "Flip screen, mic input, mini tripod."),
   (" Instant Print", 899900, 22, "Point-shoot-print retro camera."),
   (" Action 4K", 2799900, 14, "Waterproof action cam, stabilization.")]),
 ("1473968512647-3e447244af8f", "electronics", "Camera Drone", [
   ("", 4599900, 9, "4K gimbal camera, 34-min flight."),
   (" Mini SE", 2999900, 12, "Sub-249g, no licence needed."),
   (" Pro 5KM", 7499900, 6, "5km range, obstacle sensing."),
   (" Kids Starter", 899900, 25, "Prop guards, one-key return.")]),
 ("1587829741301-dc798b83add3", "electronics", "Mechanical Keyboard", [
   ("", 349900, 40, "Hot-swap switches, PBT keycaps."),
   (" Wireless", 449900, 30, "Tri-mode BT/2.4G/wired, gasket mount."),
   (" Tenkeyless", 299900, 45, "Compact TKL, south-facing RGB."),
   (" Silent Office", 279900, 50, "Silent switches for shared desks.")]),
 ("1527864550417-7fd91fc51a46", "electronics", "Wireless Mouse", [
   ("", 129900, 70, "Silent clicks, 18-month battery."),
   (" Ergonomic", 199900, 45, "Vertical grip for wrist comfort."),
   (" Gaming 26K DPI", 349900, 30, "Esports sensor, 8 buttons."),
   (" Travel Mini", 89900, 90, "Retractable receiver, ambidextrous.")]),
 ("1544244015-0df4b3ffc6b0", "electronics", "Tablet", [
   (" 10.4-inch", 1899900, 20, "2K display, quad speakers, 8GB RAM."),
   (" Pro 11-inch", 3299900, 12, "120Hz, stylus support, all-metal."),
   (" Kids Edition", 1299900, 30, "Bumper case + parental controls."),
   (" E-Reader 8-inch", 999900, 25, "Eye-care display, weeks of battery.")]),
 ("1606144042614-b2417e99c4e3", "electronics", "Gaming Console", [
   ("", 4999900, 10, "4K gaming, ultra-fast SSD, one controller."),
   (" Digital Edition", 4499900, 12, "Disc-free digital library console."),
   (" + Extra Controller", 5499900, 8, "Bundle with second controller."),
   (" VR Bundle", 8999900, 5, "Console + VR headset starter kit.")]),
 # ---------------- fashion (17 photos) ----------------
 ("1521572163474-6864f9cf17ab", "fashion", "Cotton T-Shirt White", [
   ("", 49900, 200, "Pure-cotton white tee, bio-washed."),
   (" Pack of 2", 89900, 150, "Two white tees, sizes S-XXL."),
   (" Pack of 3", 129900, 120, "Everyday value triple pack."),
   (" Oversized", 69900, 100, "Drop-shoulder oversized fit.")]),
 ("1542272604-787c3835535d", "fashion", "Slim-Fit Jeans", [
   ("", 149900, 120, "Stretch-denim slim jeans, mid-rise."),
   (" Dark Wash", 159900, 100, "Rinse-dark wash, minimal fading."),
   (" Distressed", 179900, 80, "Light distressing, tapered leg."),
   (" Straight Fit", 149900, 110, "Classic straight leg, all sizes.")]),
 ("1572635196237-14b3f281503f", "fashion", "Sunglasses", [
   (" Aviator", 129900, 90, "UV400 aviators, metal frame."),
   (" Wayfarer", 119900, 100, "Timeless wayfarer, polarized option."),
   (" Round Retro", 139900, 70, "Retro rounds with spring hinges."),
   (" Sport Wrap", 159900, 60, "Wraparound sports frame, grippy nose.")]),
 ("1584917865442-de89df76afd3", "fashion", "Handbag", [
   ("", 199900, 60, "Structured tote with zip pouch."),
   (" Mini Crossbody", 149900, 80, "Compact crossbody, adjustable strap."),
   (" Office Satchel", 249900, 40, "Fits 14-inch laptop, magnetic flap."),
   (" Evening Clutch", 179900, 50, "Slim clutch with chain strap.")]),
 ("1553062407-98eeb64c6a62", "fashion", "Backpack", [
   (" 25L", 149900, 100, "Water-repellent 25L with laptop sleeve."),
   (" 35L Travel", 199900, 70, "Front-load travel pack, rain cover."),
   (" School 20L", 99900, 140, "Light school bag, 3 compartments."),
   (" Hiking 45L", 279900, 40, "Ventilated back, hip belt.")]),
 ("1541643600914-78b084683601", "fashion", "Perfume", [
   (" Citrus 100ml", 149900, 70, "Fresh citrus daily wear."),
   (" Oud Intense", 299900, 35, "Long-lasting evening oud."),
   (" Floral 50ml", 99900, 90, "Soft floral, office-friendly."),
   (" Musk Duo", 199900, 55, "Two 50ml musks, gift box.")]),
 ("1524592094714-0f0654e20314", "fashion", "Analog Watch", [
   ("", 249900, 60, "Minimal dial, genuine leather strap."),
   (" Chronograph", 399900, 35, "Triple-subdial chrono, steel body."),
   (" Rose-Gold", 299900, 45, "Rose-gold case, mesh strap."),
   (" Field Automatic", 599900, 20, "Automatic movement, lume hands.")]),
 ("1523275335684-37898b6baf30", "fashion", "Classic Watch", [
   (" Tan Strap", 279900, 50, "Tan leather, white dial."),
   (" Black Dial", 299900, 45, "Black sunray dial, steel strap."),
   (" Couple Set", 499900, 25, "Matching pair, gift boxed."),
   (" Slim Gold", 349900, 30, "7mm slim case, gold finish.")]),
 ("1595777457583-95e059d581b8", "fashion", "Evening Dress Red", [
   ("", 249900, 40, "Flowing red maxi, festive-ready."),
   (" Midi", 199900, 55, "Knee-length red midi, pockets."),
   (" Gown", 399900, 20, "Floor-length gown, trail cut."),
   (" Wrap Mini", 179900, 60, "Red wrap dress, adjustable tie.")]),
 ("1558769132-cb1aea458c5e", "fashion", "Handloom Cotton Fabric", [
   (" per metre", 29900, 300, "Breathable handloom, blouse/curtain grade."),
   (" 3-metre cut", 79900, 150, "Dress-length cut piece."),
   (" 5-metre cut", 129900, 100, "Suit-length cut piece."),
   (" Varsity bundle", 49900, 200, "Assorted 1m craft cuts.")]),
 ("1599643478518-a784e5dc4c8f", "fashion", "Ring", [
   (" Solitaire", 99900, 80, "Zircon solitaire, adjustable band."),
   (" Band Set of 3", 149900, 60, "Stackable bands, gold finish."),
   (" Gemstone", 199900, 40, "Coloured-stone cocktail ring."),
   (" Couple Bands", 249900, 30, "Matching pair, engraved.")]),
 ("1611591437281-460bfbe1220a", "fashion", "Gold-Plated Ring", [
   ("", 79900, 90, "Gold-plated band, tarnish-safe."),
   (" Temple Design", 129900, 50, "Temple-work dome ring."),
   (" Pearl Drop", 149900, 40, "Shell-pearl accent ring."),
   (" Festive Set of 2", 199900, 35, "Two festive rings, gift pouch.")]),
 ("1573408301185-9146fe634ad0", "fashion", "Tennis Bracelet", [
   ("", 199900, 45, "Zircon tennis line, secure clasp."),
   (" Gold Finish", 249900, 35, "Gold-finish tennis bracelet."),
   (" Charm Bangle", 149900, 60, "Open bangle with charms."),
   (" Kada", 179900, 50, "Classic kada, daily wear.")]),
 ("1588850561407-ed78c282e89b", "fashion", "Cap", [
   ("", 49900, 150, "Cotton twill cap, adjustable."),
   (" Snapback", 69900, 110, "Flat-brim snapback."),
   (" Sports Dry-Fit", 79900, 90, "Sweat-wicking training cap."),
   (" Wool Winter", 89900, 70, "Warm winter cap, fleece-lined.")]),
 ("1627123424574-724758594e93", "fashion", "Leather Wallet", [
   ("", 89900, 120, "Bifold with 8 card slots, RFID safe."),
   (" Trifold", 109900, 90, "Trifold, coin pocket, gift box."),
   (" Card Holder", 59900, 150, "Slim 6-card holder."),
   (" Long Clutch", 129900, 70, "Long wallet, phone slot.")]),
 ("1586350977771-b3b0abd50c82", "fashion", "Printed Socks", [
   (" Pack of 3", 29900, 250, "Combed-cotton printed socks."),
   (" Pack of 5", 44900, 200, "Weekday print rotation pack."),
   (" Ankle Pack of 4", 34900, 220, "Low-cut ankle prints."),
   (" Knee-High", 39900, 150, "Fun knee-high prints.")]),
 ("1534274988757-a28bf1a57c17", "fashion", "Umbrella", [
   (" Compact", 59900, 130, "Windproof compact, auto open-close."),
   (" Golf 30-inch", 99900, 80, "Large canopy, wooden handle."),
   (" Transparent", 79900, 90, "Clear dome, city-rain style."),
   (" Kids Print", 49900, 140, "Light kids prints, safety tip.")]),
 # ---------------- footwear (3 photos) ----------------
 ("1542291026-7eec264c27ff", "footwear", "Running Shoes Red", [
   ("", 299900, 70, "Cushioned red runners, breathable mesh."),
   (" Marathon Pro", 499900, 35, "Carbon-plate racer, featherlight."),
   (" Trail Grip", 349900, 45, "Lugged trail outsole, rock plate."),
   (" Daily Trainer", 249900, 80, "Balanced daily trainer.")]),
 ("1549298916-b41d501d3772", "footwear", "Sneakers White", [
   ("", 199900, 110, "Clean white street sneakers."),
   (" Court Classic", 229900, 90, "Cupsole courts, leather upper."),
   (" Slip-On", 179900, 100, "Elastic slip-on, travel pair."),
   (" Platform", 249900, 60, "Chunky platform sole.")]),
 ("1595950653106-6c9ebd614d3a", "footwear", "Sneakers Pastel", [
   ("", 219900, 85, "Pastel colour-block sneakers."),
   (" KnitFlex", 239900, 70, "Sock-knit pastel runners."),
   (" Retro 80s", 259900, 55, "Retro runner, pastel panels."),
   (" School White", 149900, 130, "Uniform-friendly school sneakers.")]),
 # ---------------- home (15 photos) ----------------
 ("1555041469-a586c61ea9bc", "home", "Fabric Sofa", [
   (" 3-Seater", 2499900, 8, "Deep-seat 3-seater, washable covers."),
   (" 5-Seater L", 3999900, 5, "L-shaped 5-seater for families."),
   (" 2-Seater Loveseat", 1799900, 10, "Compact loveseat for small flats."),
   (" Futon Convertible", 1499900, 12, "Sofa-cum-bed, easy fold.")]),
 ("1503602642458-232111445657", "home", "Accent Chair White", [
   ("", 899900, 18, "Moulded white shell, wood legs."),
   (" Rocking", 1099900, 12, "Rocking version, nursery-ready."),
   (" Outdoor", 799900, 22, "Weather-proof patio edition."),
   (" Kids Mini", 499900, 30, "Child-size mini chair.")]),
 ("1592078615290-033ee584e267", "home", "Accent Chair Black", [
   ("", 949900, 16, "Matte-black lounge chair, wood legs."),
   (" Office Visitor", 849900, 20, "Visitor chair for home office."),
   (" Cafe Set of 2", 1699900, 10, "Two chairs, cafe pairing."),
   (" Velvet Cushion", 1099900, 14, "Velvet seat pad included.")]),
 ("1507473885765-e6ed057f782c", "home", "Desk Lamp", [
   ("", 149900, 70, "Warm study lamp, touch dimmer."),
   (" Architect Arm", 229900, 45, "Swing-arm architect lamp."),
   (" Clip-On", 99900, 90, "Clip bedside lamp, USB powered."),
   (" Vintage Edison", 199900, 50, "Edison-bulb vintage look.")]),
 ("1505693416388-ac5ce068fe85", "home", "Bedding Set", [
   (" Queen 210TC", 199900, 60, "Glace-cotton queen set, 4pc."),
   (" King 300TC", 299900, 40, "Sateen king set, deep pockets."),
   (" Single Duo", 149900, 80, "Two single sets, hostel-ready."),
   (" Festive Gold", 349900, 30, "Gold-jacquard festive bedding.")]),
 ("1522771739844-6a9f6d5f14af", "home", "Hotel Bedding Collection", [
   ("", 279900, 45, "Crisp hotel-white bedding set."),
   (" Blue Accent", 299900, 35, "White set with blue cushions."),
   (" Winter Comforter", 399900, 25, "220GSM comforter + covers."),
   (" Summer Cool", 229900, 50, "Percale summer set, breathable.")]),
 ("1513694203232-719a280e022f", "home", "Living-Room Decor Set", [
   ("", 199900, 40, "Cushions + throw + plant pot combo."),
   (" Green Corner", 249900, 30, "Two planters + floor cushion."),
   (" Festive", 299900, 25, "Diyas + toran + cushion covers."),
   (" Minimal", 149900, 55, "Single-tone minimal set.")]),
 ("1563861826100-9cb868fdbe1c", "home", "Wall Clock", [
   ("", 129900, 80, "Silent-sweep 12-inch clock."),
   (" Vintage Roman", 179900, 55, "Roman-numeral vintage dial."),
   (" Kitchen 8-inch", 89900, 100, "Compact kitchen clock."),
   (" Designer Duo", 249900, 35, "Two designer clocks, gift box.")]),
 ("1513519245088-0e12902e5a38", "home", "Photo Frames Set", [
   (" 6pc Gallery", 99900, 110, "Mixed-size gallery wall set."),
   (" 12pc Value", 169900, 70, "Full-wall 12-frame kit."),
   (" A4 Certificate", 49900, 150, "Document frames with mounts."),
   (" LED Backlit", 249900, 30, "Backlit display frame.")]),
 ("1618220179428-22790b461013", "home", "Armchair Orange", [
   ("", 1299900, 10, "Bold orange lounge armchair."),
   (" Reading Pair", 2499900, 6, "Two armchairs + side table."),
   (" Velvet", 1599900, 8, "Velvet upholstery edition."),
   (" Rocking", 1499900, 9, "Rocking armchair, wood frame.")]),
 ("1600166898405-da9535204843", "home", "Wool Rug", [
   (" 4x6ft", 299900, 35, "Hand-tufted wool rug."),
   (" Runner 2x6ft", 199900, 45, "Hallway runner, non-slip."),
   (" Round 4ft", 249900, 30, "Round centre rug."),
   (" Prayer Mat", 89900, 90, "Soft prayer rug, tasselled.")]),
 ("1485955900006-10f4d324d411", "home", "Potted Plant", [
   (" Money Plant", 39900, 150, "Lucky money plant in ceramic pot."),
   (" Snake Plant", 49900, 130, "Air-purifying snake plant."),
   (" Succulent Trio", 59900, 120, "Three succulents, mini pots."),
   (" Areca Palm", 89900, 80, "2ft areca palm, planter.")]),
 ("1581578731548-c64695cc6952", "home", "Home Cleaning Kit", [
   ("", 79900, 140, "Sprays + microfibres + brush set."),
   (" Deep-Clean", 129900, 90, "Degreaser + descaler + mop pads."),
   (" Eco Refill", 99900, 110, "Plant-based refills, no plastic."),
   (" Bathroom Pro", 89900, 120, "Mould remover + squeegee.")]),
 ("1620626011761-996317b8d101", "home", "Bath Linen Set", [
   ("", 129900, 100, "2 bath + 2 hand towels, 600GSM."),
   (" Family 6pc", 199900, 70, "Full-family towel set."),
   (" Guest Duo", 89900, 130, "Guest towel pair, gift wrap."),
   (" Kids Hooded", 99900, 110, "Hooded kids towel, fun prints.")]),
 ("1631049307264-da0ec9d70304", "home", "Bed Pillows Hotel Set", [
   (" Set of 2", 99900, 120, "Microfibre hotel pillows."),
   (" Memory Foam", 199900, 70, "Contoured memory-foam pair."),
   (" Sleeping + Cushion", 149900, 90, "2 sleep + 2 cushion covers."),
   (" Ortho Cervical", 229900, 50, "Neck-support ortho pillow.")]),
 # ---------------- kitchen (4 photos) ----------------
 ("1594213114663-d94db9b17125", "kitchen", "Steel Kettle", [
   (" 1.5L", 89900, 110, "Whistling steel kettle, gas+induction."),
   (" 2.5L Family", 129900, 80, "Large family kettle."),
   (" Electric 1.8L", 199900, 60, "Cordless electric kettle."),
   (" Camping 1L", 109900, 90, "Compact trail kettle.")]),
 ("1602143407151-7111542de6e8", "kitchen", "Insulated Flask Green", [
   (" 1L", 89900, 140, "24h hot/cold steel flask."),
   (" 750ml Duo", 139900, 100, "Two 750ml flasks, office pair."),
   (" Kids 500ml", 69900, 150, "Leak-proof kids flask."),
   (" 2L Family", 129900, 80, "Big family flask with cups.")]),
 ("1546069901-ba9599a7e63c", "kitchen", "Serving Bowl", [
   ("", 59900, 130, "Everyday serving bowl, 1.2L."),
   (" Set of 3", 149900, 80, "Nesting bowl trio."),
   (" Salad Big", 89900, 100, "Large salad/fruit bowl."),
   (" Steel Set of 2", 99900, 110, "Steel serving tokris.")]),
 ("1585515320310-259814833e62", "kitchen", "Mixer-Blender Jar", [
   (" 1000W", 349900, 40, "1000W with 3 jars, chutney blade."),
   (" 750W Value", 249900, 60, "3-jar value mixer grinder."),
   (" Juicer Combo", 399900, 30, "Mixer + centrifugal juicer."),
   (" Compact 500W", 199900, 70, "Bachelor-size 2-jar mixer.")]),
 # ---------------- grocery (10 photos) ----------------
 ("1447933601403-0c6688de566e", "grocery", "Coffee Beans", [
   (" Arabica 500g", 49900, 120, "Medium-roast arabica beans."),
   (" Robusta 1kg", 79900, 90, "Strong robusta for decoction."),
   (" Peaberry 250g", 39900, 110, "Peaberry single-estate."),
   (" Espresso 1kg", 89900, 80, "Dark espresso roast.")]),
 ("1509042239860-f550ce710b93", "grocery", "Filter Coffee Powder", [
   (" 80/20 500g", 34900, 200, "Classic coffee-chicory blend."),
   (" Pure 250g", 29900, 180, "100% coffee, no chicory."),
   (" 1kg Family", 64900, 120, "Family pack, fresh roast."),
   (" Trial 100g", 14900, 250, "Taster pack.")]),
 ("1544787219-7f47ccb76574", "grocery", "Premium Tea", [
   (" Assam 250g", 29900, 200, "Malty Assam CTC."),
   (" Masala 250g", 34900, 180, "Spiced masala chai blend."),
   (" Green 100 bags", 39900, 150, "Green tea bags, 100pc."),
   (" 1kg Value", 99900, 100, "Canteen-size tea pack.")]),
 ("1596040033229-a9821ebd058d", "grocery", "Masala Spice Box", [
   ("", 49900, 160, "7-essential spice dabba set."),
   (" Garam + Sambar", 34900, 180, "Two fresh blends."),
   (" Whole Spices 500g", 59900, 120, "Sabut masala combo."),
   (" Pavilion Gift", 89900, 80, "Gift-ready spice chest.")]),
 ("1586201375761-83865001e31c", "grocery", "Basmati Rice", [
   (" 5kg", 64900, 140, "Long-grain aged basmati."),
   (" 1kg Trial", 15900, 250, "Taster kilo."),
   (" 25kg Sack", 289900, 40, "Marriage-hall sack."),
   (" Brown 5kg", 74900, 100, "Unpolished brown basmati.")]),
 ("1560806887-1e4cd0b6cbd6", "grocery", "Farm Apples", [
   (" 1kg", 19900, 220, "Crisp Shimla apples."),
   (" 2kg", 37900, 180, "Family 2kg box."),
   (" 4kg Orchard", 69900, 120, "Orchard box, graded."),
   (" Juice Pack 5kg", 79900, 100, "Juicing-grade apples.")]),
 ("1540420773420-3366772f4999", "grocery", "Fresh Vegetables Combo", [
   (" 3kg", 29900, 200, "Seasonal sabzi mix."),
   (" 5kg Family", 44900, 150, "Week's family vegetables."),
   (" Salad Kit", 19900, 220, "Ready salad vegetables."),
   (" Exotic 2kg", 59900, 100, "Zucchini, broccoli, peppers.")]),
 ("1508747703725-719777637510", "grocery", "Red Onions", [
   (" 1kg", 5900, 300, "Everyday red onions."),
   (" 5kg", 24900, 200, "Monthly 5kg bag."),
   (" 25kg Sack", 109900, 60, "Trader sack."),
   (" Shallots 500g", 4900, 250, "Sambar shallots.")]),
 ("1587049352846-4a222e784d38", "grocery", "Fresh Watermelon", [
   (" Whole 3kg", 12900, 180, "Sweet whole melon."),
   (" Half Cut", 7900, 220, "Ready half, cling-wrapped."),
   (" Cubes 500g", 9900, 250, "Cut cubes, snack box."),
   (" Juice 1L", 14900, 150, "Cold-pressed melon juice.")]),
 ("1474979266404-7eaacbcd87c5", "grocery", "Cold-Pressed Oil", [
   (" Groundnut 1L", 24900, 200, "Wood-pressed groundnut oil."),
   (" Sesame 1L", 29900, 170, "Chekku sesame oil."),
   (" Coconut 1L", 27900, 180, "Cold-pressed coconut oil."),
   (" Mustard 1L", 22900, 190, "Kachi-ghani mustard oil.")]),
 # ---------------- beauty (4 photos) ----------------
 ("1586495777744-4413f21062fa", "beauty", "Lipstick", [
   (" Matte Red", 49900, 140, "Long-stay matte red."),
   (" Nude Set of 3", 99900, 100, "Everyday nude trio."),
   (" Maroon Classic", 54900, 120, "Classic maroon bullet."),
   (" Mini Combo", 69900, 130, "4 minis, festive shades.")]),
 ("1556228720-195a672e8a03", "beauty", "Face Serum", [
   (" Vitamin C", 79900, 110, "Brightening Vit-C serum."),
   (" Hyaluronic", 89900, 90, "Deep-hydration HA serum."),
   (" Retinol Night", 99900, 80, "Beginner-friendly retinol."),
   (" Glow Duo", 149900, 60, "Day + night serum pair.")]),
 ("1600857544200-b2f666a9a2ec", "beauty", "Handmade Soap Pack", [
   (" 3pc", 29900, 200, "Cold-process bars, assorted."),
   (" Charcoal 4pc", 39900, 160, "Detox charcoal bars."),
   (" Baby Mild 3pc", 34900, 180, "Tear-free baby bars."),
   (" Gift Hamper", 59900, 100, "6 bars + loofah box.")]),
 ("1620916566398-39f1143ab7be", "beauty", "Face Cream", [
   (" Day 50g", 49900, 150, "Light day moisturiser, SPF."),
   (" Night Repair", 69900, 120, "Overnight repair cream."),
   (" Winter Cold", 59900, 140, "Rich winter cream."),
   (" Oil-Free Gel", 54900, 130, "Gel moisturiser, oily skin.")]),
 # ---------------- sports (6 photos) ----------------
 ("1485965120184-e220f721d03e", "sports", "City Bicycle", [
   (" Single-Speed", 899900, 20, "Simple city single-speed."),
   (" 21-Gear", 1299900, 14, "Shimano 21-gear commuter."),
   (" Kids 20-inch", 699900, 25, "Kids cycle with trainer wheels."),
   (" Electric Assist", 3499900, 6, "E-cycle, 60km range.")]),
 ("1517836357463-d25dfeac3438", "sports", "Dumbbell Set", [
   (" 10kg Pair", 149900, 70, "Rubber-coated hex pair."),
   (" Adjustable 24kg", 499900, 30, "Dial-adjustable single."),
   (" 5kg Beginners", 89900, 100, "Neoprene beginner pair."),
   (" Home Gym 20kg", 299900, 45, "Plates + rods starter kit.")]),
 ("1579952363873-27f3bade9f55", "sports", "Football", [
   (" Size 5", 89900, 110, "Match size-5, all-weather."),
   (" Training", 49900, 150, "Training ball, machine-stitched."),
   (" Futsal Size 4", 69900, 90, "Low-bounce futsal ball."),
   (" Kids Size 3", 39900, 160, "Light kids ball.")]),
 ("1531415074968-036ba1b575da", "sports", "Leather Cricket Ball", [
   (" 2pc", 49900, 140, "Season 2-piece ball."),
   (" 4pc Tournament", 89900, 90, "Alum-tanned 4-piece."),
   (" Tennis Cricket 6pc", 39900, 180, "Heavy tennis balls."),
   (" Practice 12pc", 99900, 70, "Net-practice dozen.")]),
 ("1626224583764-f87db24ac4ea", "sports", "Badminton Racket", [
   (" Graphite", 129900, 80, "Full-graphite feather racket."),
   (" Aluminium Pair", 99900, 100, "Two-racket family set."),
   (" Pro Isometric", 249900, 45, "Isometric head, high tension."),
   (" Kids Short", 69900, 130, "Short kids racket + shuttle.")]),
 ("1593095948071-474c5cc2989d", "sports", "Whey Protein", [
   (" Chocolate 1kg", 249900, 60, "24g protein per scoop."),
   (" Vanilla 500g", 139900, 90, "Starter half-kilo."),
   (" Isolate 1kg", 329900, 40, "27g isolate, low lactose."),
   (" Mass Gainer 3kg", 399900, 30, "High-calorie gainer.")]),
 # ---------------- books (4 photos) ----------------
 ("1544947950-fa07a98d237f", "books", "Hardcover Journal", [
   ("", 49900, 150, "192-page lay-flat journal."),
   (" Lined + Dotted", 59900, 120, "Reversible lined/dotted."),
   (" Gift Boxed", 79900, 90, "Journal + pen gift set."),
   (" A6 Pocket", 29900, 180, "Pocket A6 daily diary.")]),
 ("1512820790803-83ca734da794", "books", "Bestseller Combo 3 Books", [
   (" Fiction", 79900, 110, "Three chart-topping novels."),
   (" Self-Help", 89900, 100, "Habits + focus + calm trio."),
   (" Kids Stories", 69900, 130, "Illustrated kids triple."),
   (" Classics", 99900, 80, "Three timeless classics.")]),
 ("1543002588-bfa74002ed7e", "books", "Memory Scrapbook", [
   ("", 59900, 120, "Photo-safe scrapbook, 40 pages."),
   (" Wedding", 99900, 70, "Ivory wedding scrapbook."),
   (" Baby First-Year", 89900, 80, "Milestone baby book."),
   (" Travel", 69900, 100, "Pocket travel log.")]),
 ("1531346878377-a5be20888e57", "books", "Spiral Notebook", [
   (" Pack of 3", 29900, 250, "200-page spirals, ruled."),
   (" Executive", 49900, 150, "Hard-cover executive pad."),
   (" Graph Pack of 2", 34900, 200, "Maths graph books."),
   (" Steno 4pc", 24900, 250, "Pocket steno pads.")]),
 # ---------------- toys (6 photos) ----------------
 ("1587654780291-39c9404d746b", "toys", "Building Blocks", [
   (" 250pc", 89900, 110, "Classic bricks tub."),
   (" 500pc", 149900, 70, "Deluxe builder box."),
   (" Vehicle Set", 119900, 80, "Cars + planes builds."),
   (" Pink Creator", 99900, 90, "Pastel creator box.")]),
 ("1594787318286-3d835c1d207f", "toys", "Die-Cast Toy Car", [
   (" Vintage Beetle", 69900, 100, "Pull-back vintage car."),
   (" Set of 4", 199900, 60, "Four-car city set."),
   (" RC Stunt", 149900, 50, "Remote stunt car."),
   (" Truck Hauler", 129900, 70, "Carrier truck + 2 cars.")]),
 ("1515488042361-ee00e0ddd4e4", "toys", "Kids Learning Toy Set", [
   ("", 79900, 120, "Puzzle + plush learning combo."),
   (" Phonics", 99900, 90, "Alphabet + number toys."),
   (" Art Starters", 69900, 130, "Crayons + colouring + stickers."),
   (" Blocks + Books", 119900, 80, "Blocks with picture books.")]),
 ("1513364776144-60967b0f800f", "toys", "Artist Brush Set", [
   (" 12pc", 49900, 150, "Taklon brushes, roll case."),
   (" 24pc Pro", 89900, 90, "Full round/flat/filbert set."),
   (" Kids Washable", 39900, 180, "Easy-clean kids brushes."),
   (" Watercolour", 69900, 110, "Mop + liner watercolour set.")]),
 ("1510915361894-db8b60106cb1", "toys", "Acoustic Guitar", [
   (" Beginner", 349900, 30, "Full-size learner + bag."),
   (" 3/4 Kids", 299900, 35, "Small hands starter."),
   (" Cutaway Electric", 549900, 18, "Cutaway with pickup."),
   (" Classical Nylon", 399900, 25, "Soft nylon strings.")]),
 ("1520523839897-bd0b52f945a0", "toys", "Keyboard Piano", [
   (" 61-Key", 799900, 15, "Touch-response 61 keys."),
   (" 88-Key Weighted", 2499900, 6, "Hammer-action home piano."),
   (" Kids 37-Key", 399900, 30, "Light-up kids keys."),
   (" MIDI Controller", 999900, 12, "25-key USB MIDI pad.")]),
 # ---------------- appliances (1 photo) ----------------
 ("1610557892470-55d9e80c0bce", "appliances", "Front-Load Washing Machine", [
   (" 7kg", 3299900, 8, "Steam wash, 15-min quick."),
   (" 8kg Inverter", 3799900, 6, "Direct-drive, 10-yr warranty."),
   (" 6kg Compact", 2899900, 10, "Slim 6kg for small flats."),
   (" 9kg Family", 4299900, 5, "Large family drum, dryer combo.")]),
]

def main():
    c = psycopg.connect(DSN)
    c.autocommit = True
    s = c.execute("SELECT id FROM users WHERE email='seller@sanghavimart.local'").fetchone()
    if not s:
        print("demo seller missing — run db/seed.sql first")
        sys.exit(1)
    sid = s[0]
    ins = upd = 0
    for pid, cat, base, variants in CAT:
        for suffix, price, stock, desc in variants:
            name = base + suffix
            url = IMG % pid
            r = c.execute("UPDATE products SET description=%s,price_cents=%s,stock_qty=%s,category=%s,image_url=%s WHERE seller_id=%s AND name=%s",
                          (desc, price, stock, cat, url, sid, name))
            if r.rowcount:
                upd += 1
            else:
                c.execute("INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url) VALUES(%s,%s,%s,%s,%s,%s,%s)",
                          (sid, name, desc, price, stock, cat, url))
                ins += 1
    n = c.execute("SELECT COUNT(*), COUNT(DISTINCT category) FROM products WHERE seller_id=%s", (sid,)).fetchone()
    print(f"catalog: inserted={ins} updated={upd} | seller total={n[0]} across {n[1]} categories")
    rows = c.execute("SELECT category, COUNT(*) FROM products WHERE seller_id=%s GROUP BY 1 ORDER BY 1", (sid,)).fetchall()
    for cat, k in rows:
        print(f"  {cat}: {k}")
    c.close()

main()
