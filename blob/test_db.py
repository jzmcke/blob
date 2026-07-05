import sqlite3
import streamlit as st

# Configure the page layout
st.set_page_config(page_title="Food Nutrient Estimator", layout="centered")
st.title("🥑 Food Nutrient Search Engine")
conn = sqlite3.connect(r"C:\Users\Joe\Downloads\FoodData_Central_csv_2025-12-18\FoodData_Central_csv_2025-12-18\usda.db")
cursor = conn.cursor()
tables = cursor.execute("SELECT name FROM sqlite_master WHERE type='table';").fetchall()
conn.close()

st.warning("⚠️ Checking your database... Here are the actual tables inside usda.db:")
st.write([t[0] for t in tables])
st.write("---")
st.caption("Powered by USDA FoodData Central & SQLite")

# 1. Establish Database Connection Connection
def get_db_connection():
    conn = sqlite3.connect(r"C:\Users\Joe\Downloads\FoodData_Central_csv_2025-12-18\FoodData_Central_csv_2025-12-18\usda.db")
    conn.row_factory = sqlite3.Row  # Access columns by name string dicts
    return conn

# 2. Search Box Inputs
query = st.text_input("Search for food items or ingredients:", placeholder="e.g., Nature's Path Waffles")

if query:
    conn = get_db_connection()
    cursor = conn.cursor()

    sql_search = """
        SELECT 
            f.fdc_id, 
            f.description, 
            f.data_type,
            b.serving_size, 
            b.serving_size_unit, 
            b.household_serving_fulltext,
            MAX(CASE WHEN fn.nutrient_id = 1003 THEN fn.amount END) AS protein_100g,
            MAX(CASE WHEN fn.nutrient_id = 1004 THEN fn.amount END) AS fat_100g,
            MAX(CASE WHEN fn.nutrient_id = 1005 THEN fn.amount END) AS carbs_100g,
            MAX(CASE WHEN fn.nutrient_id = 1093 THEN fn.amount END) AS sodium_100g,
            MAX(CASE WHEN fn.nutrient_id = 1079 THEN fn.amount END) AS fiber_100g, 
            MAX(CASE WHEN fn.nutrient_id = 1258 THEN fn.amount END) AS sat_fat_100g,
            MAX(CASE WHEN fn.nutrient_id = 1257 THEN fn.amount END) AS trans_fat_100g,
            MAX(CASE WHEN fn.nutrient_id = 1292 THEN fn.amount END) AS mono_fat_100g,
            MAX(CASE WHEN fn.nutrient_id = 1293 THEN fn.amount END) AS poly_fat_100g
        FROM (
            SELECT m.fdc_id, m.description, m.data_type 
            FROM flattened_food_macros m
            JOIN flattened_food_macros_fts fts ON m.rowid = fts.rowid
            WHERE fts.description MATCH ? 
            ORDER BY 
                -- 1. Force raw and generic ingredients to the top
                CASE 
                    WHEN m.data_type = 'foundation_food' THEN 1
                    WHEN m.data_type = 'sr_legacy_food' THEN 2
                    WHEN m.data_type = 'survey_fndds_food' THEN 3
                    ELSE 4 
                END,
                -- 2. Then sort by text match accuracy
                fts.rank
            LIMIT 15
        ) f
        LEFT JOIN branded_food b ON f.fdc_id = b.fdc_id
        LEFT JOIN food_nutrient fn ON f.fdc_id = fn.fdc_id
        GROUP BY f.fdc_id
        ORDER BY 
            CASE 
                WHEN f.data_type = 'foundation_food' THEN 1
                WHEN f.data_type = 'sr_legacy_food' THEN 2
                WHEN f.data_type = 'survey_fndds_food' THEN 3
                ELSE 4 
            END,
            f.description;
    """
    
    safe_query = query.replace('"', '""')
    results = cursor.execute(sql_search, (f'"{safe_query}"*',)).fetchall()
    conn.close()

    if results:
        # Turn database records into a clean select box dictionary list
        food_options = {f"{r['description']} [{r['data_type'].upper()}]": r for r in results}
        selected_key = st.selectbox("Select matching food item:", options=list(food_options.keys()))
        
        selected_food = food_options[selected_key]
        
        st.write("---")
        st.subheader(selected_food['description'])
        
        # Pull base baseline details (Default fallback to 100g if data fields are blank)
        base_serving = float(selected_food['serving_size']) if selected_food['serving_size'] else 100.0
        unit = selected_food['serving_size_unit'] if selected_food['serving_size_unit'] else "g"
        household_desc = selected_food['household_serving_fulltext'] if selected_food['household_serving_fulltext'] else ""

        if household_desc:
            st.info(f"**Standard Serving Size Reference:** {base_serving} {unit} ({household_desc})")
        else:
            st.info(f"**Standard Serving Size Reference:** {base_serving} {unit}")

        # 3. Dynamic Multiplier Inputs
        weight_input = st.number_input(f"Enter consumption amount ({unit}):", min_value=1.0, value=base_serving, step=1.0)
        
        # Ratio scalar math logic
        scalar = weight_input / 100.0

        def calc(val):
            return round(float(val) * scalar, 2) if val is not None else 0.0

        # 4. Display Results Cards
        st.metric(label="Calculated Logging Weight", value=f"{weight_input} {unit}")
        
        col1, col2, col3, col4 = st.columns(4)
        col1.metric("Protein", f"{calc(selected_food['protein_100g'])} g")
        col2.metric("Total Fats", f"{calc(selected_food['fat_100g'])} g")
        col3.metric("Carbs", f"{calc(selected_food['carbs_100g'])} g")
        col4.metric("Sodium", f"{calc(selected_food['sodium_100g'])} mg")

        # Lipid Sub-Table Details
        st.markdown("### 🍳 Lipid Profile Breakdown")
        st.json({
            "saturated_fat_g": calc(selected_food['sat_fat_100g']),
            "trans_fat_g": calc(selected_food['trans_fat_100g']),
            "monounsaturated_fat_g": calc(selected_food['mono_fat_100g']),
            "polyunsaturated_fat_g": calc(selected_food['poly_fat_100g'])
        })
    else:
        st.warning("No matches found. Try a different keyword descriptor combination.")