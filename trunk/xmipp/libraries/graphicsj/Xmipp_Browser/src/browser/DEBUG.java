/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package browser;

/**
 *
 * @author Juanjo Vega
 */
public class DEBUG {

    private final static boolean DEBUG = true;

    public static void printMessage(String message) {
        if (DEBUG) {
            System.out.println(message);
        }
    }
}
