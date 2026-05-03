import pygetwindow as gw
import pyautogui
import time
import sys

def automate_window():
    # 1. Get and filter window titles
    all_windows = gw.getAllTitles()
    # Filter out empty titles (background processes)
    titles = [t for t in all_windows if t.strip()]

    if not titles:
        print("No active windows found.")
        return

    print("--- Currently Opened Windows ---")
    for i, title in enumerate(titles):
        print(f"[{i}] {title}")

    # 2. User selection
    try:
        choice = int(input("\nEnter the number of the window you want to target: "))
        target_title = titles[choice]
        print(f"Target locked: '{target_title}'")
    except (ValueError, IndexError):
        print("Invalid selection. Please run the script again and enter a valid number.")
        sys.exit()

    # 3. The 4-hour wait
    # Calculation: 4 hours * 60 minutes * 60 seconds = 14,400 seconds
    wait_seconds = 14400 
    
    print(f"\nScript is now idling for 4 hours. See you at {time.ctime(time.time() + wait_seconds)}.")
    print("Keep this terminal open and ensure your computer does not go to sleep.")
    
    time.sleep(wait_seconds)

    # 4. Bring to focus and type
    try:
        # Find the window object
        window = gw.getWindowsWithTitle(target_title)[0]
        
        # Restore if minimized and bring to front
        if window.isMinimized:
            window.restore()
        window.activate()
        
        # Short pause to ensure focus is captured before typing
        time.sleep(1)
        
        pyautogui.write("Continue")
        pyautogui.press("enter")
        
        print(f"\n[{time.ctime()}] Successfully typed 'Continue' into {target_title}.")
        
    except Exception as e:
        print(f"\nError: Could not find or focus the window. It may have been closed. Details: {e}")

if __name__ == "__main__":
    automate_window()